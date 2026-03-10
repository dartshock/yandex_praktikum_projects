#include "app.h"

namespace app {
Player& Players::Add(std::shared_ptr<Dog>&& dog,
                     std::shared_ptr<GameSession>&& session) {
    CompositeKey key{*dog->GetId(), *session->GetId()};
    auto [it, inserted] =
        players_.emplace(key, Player{std::move(dog), std::move(session)});

    return it->second;
}

void Players::Remove(std::shared_ptr<Dog>&& dog,
                     std::shared_ptr<GameSession>&& session) {
    players_.erase(CompositeKey{*dog->GetId(), *session->GetId()});
}

const Player* Players::FindByDogIdAndSessionId(std::uint64_t dog_id,
                                               std::uint64_t session_id) {
    auto iter = players_.find(CompositeKey{dog_id, session_id});
    if (iter == players_.end()) {
        return nullptr;
    }

    return &iter->second;
}

const Player* PlayerTokens::FindPlayerByToken(const Token& token) {
    auto iter = token_to_player_.find(token);
    if (iter != token_to_player_.end()) {
        return iter->second;
    }
    return nullptr;
}

Token PlayerTokens::AddPlayer(Player& player) {
    auto token = GenerateToken();
    AddPlayerWithToken(player, token);
    return token;
}

void PlayerTokens::AddPlayerWithToken(Player& player, const Token& token) {
    token_to_player_.emplace(token, &player);
    player_to_token_.emplace(&player, token);
}

void PlayerTokens::RemovePlayer(const Player* player) {
    auto token = player_to_token_.at(player);
    token_to_player_.erase(token);
    player_to_token_.erase(player);
}

Token PlayerTokens::GenerateToken() {
    std::uniform_int_distribution<std::uint64_t> dist;
    std::uint64_t num1 = dist(gen_);
    std::uint64_t num2 = dist(gen_);

    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << num1
       << std::setw(16) << num2;

    return Token{ss.str()};
}

const PlayerTokens::TokensToPlayers& PlayerTokens::GetTokensToPlayers() const {
    return token_to_player_;
}

std::string ApplicationException::ToString() const {
    std::string reason;
    switch (error_) {
        case Error::Parse:
            reason = "Failed to parse: "s;
            break;
        case Error::InvalidToken:
            reason = "Invalid authorization format: "s;
            break;
        case Error::UnknownToken:
            reason = "Unknown token: "s;
            break;
        case Error::UnknownMap:
            reason = "Map not found: "s;
            break;
        case Error::Tick:
            reason = "Unable to execute tick: "s;
            break;
        case Error::DataBase:
            reason = "Data base bad request: "s;
            break;
    }
    return reason + message_;
}

Token ApplicationUseCase::GetPlayerToken(std::string_view auth_value) const {
    const std::string_view prefix = "Bearer "sv;
    if (auth_value.starts_with(prefix)) {
        std::string token{auth_value.substr(prefix.length())};
        if (token.length() == player_tokens_.GenerateToken()->length()) {
            return Token{token};
        }
    }

    throw ApplicationException(ApplicationException::Error::InvalidToken,
                               std::string(auth_value));
}

const Player* ApplicationUseCase::AuthorizePlayer(
    std::string_view auth_value) const {
    const auto* player =
        player_tokens_.FindPlayerByToken(GetPlayerToken(auth_value));
    if (player == nullptr) {
        throw ApplicationException(ApplicationException::Error::UnknownToken,
                                   "player token has not been found"s);
    }
    return player;
}

const Map* ApplicationUseCase::GetMapPtrByMapId(std::string&& map_id) const {
    auto map_ptr = game_.FindMap(model::Map::Id{map_id});
    if (!map_ptr) {
        throw ApplicationException(ApplicationException::Error::UnknownMap,
                                   std::move(map_id));
    }
    return map_ptr;
}

std::shared_ptr<GameSession> ApplicationUseCase::GetSessionByMapId(
    std::string&& map_id) const {
    return game_.GetSessionByMapPtr(GetMapPtrByMapId(std::move(map_id)));
}

json::value ApplicationUseCase::ParseJsonInput(std::string_view input) const {
    sys::error_code ec;
    auto json_value = json::parse(std::string(input), ec);
    if (ec || !json_value.is_object()) {
        throw ApplicationException(ApplicationException::Error::Parse,
                                   "wrong json format"s);
    }
    return json_value;
}

json::value MapsUseCase::GetMaps() const {
    json::array maps_arr;
    for (const auto& map : game_.GetMaps()) {
        json::value map_json{{"id"s, *map.GetId()}, {"name"s, map.GetName()}};
        maps_arr.push_back(std::move(map_json));
    }

    return maps_arr;
}

json::value MapsUseCase::GetMap(std::string&& map_id) const {
    const Map* map_ptr = GetMapPtrByMapId(std::move(map_id));

    auto road_to_json = [](const Road& road) {
        json::value road_json{{"x0"s, road.GetStart().x},
                              {"y0"s, road.GetStart().y}};
        if (road.IsHorizontal()) {
            road_json.as_object().emplace("x1"s, road.GetEnd().x);
        } else {
            road_json.as_object().emplace("y1"s, road.GetEnd().y);
        }

        return road_json;
    };

    auto building_to_json = [](const Building& building) {
        return json::value{
            {"x"s, building.GetBounds().position.x},
            {"y"s, building.GetBounds().position.y},
            {"w"s, building.GetBounds().size.width},
            {"h"s, building.GetBounds().size.height},
        };
    };

    auto office_to_json = [](const Office& office) {
        return json::value{
            {"id"s, *office.GetId()},
            {"x"s, office.GetPosition().x},
            {"y"s, office.GetPosition().y},
            {"offsetX"s, office.GetOffset().dx},
            {"offsetY"s, office.GetOffset().dy},
        };
    };

    return json::value{
        {"id"s, *map_ptr->GetId()},
        {"name"s, map_ptr->GetName()},
        {"roads"s, ContainerToJson(map_ptr->GetRoads(), road_to_json)},
        {"buildings"s,
         ContainerToJson(map_ptr->GetBuildings(), building_to_json)},
        {"offices"s, ContainerToJson(map_ptr->GetOffices(), office_to_json)},
        {"lootTypes"s, map_ptr->GetExtraData().GetData()}};
}

json::value JoinGameUseCase::Join(std::string_view join_info_json) {
    auto join_info_obj = ParseJsonInput(join_info_json).get_object();

    auto username_it = join_info_obj.find("userName"s);
    auto map_id_it = join_info_obj.find("mapId"s);

    if (username_it == join_info_obj.end()) {
        throw ApplicationException(ApplicationException::Error::Parse,
                                   "player's name is not specified"s);
    }
    if (map_id_it == join_info_obj.end()) {
        throw ApplicationException(ApplicationException::Error::Parse,
                                   "map ID is not specified"s);
    }

    return AddPlayerOnMap(username_it->value().as_string().c_str(),
                          map_id_it->value().as_string().c_str());
}

json::value JoinGameUseCase::AddPlayerOnMap(std::string&& username,
                                            std::string&& map_id) {
    if (username.empty()) {
        throw ApplicationException(ApplicationException::Error::Parse,
                                   "empty name"s);
    }

    auto session = GetSessionByMapId(std::move(map_id));
    auto spawn_point = GetSpawnPoint(*session->GetMap());

    auto& player = players_.Add(
        session->AddDog(std::move(username), std::move(spawn_point)),
        std::move(session));
    auto token = player_tokens_.AddPlayer(player);

    return json::value{{"authToken", *token}, {"playerId", *player.GetId()}};
}

DogPosition JoinGameUseCase::GetRandomPosition(const Map& map) {
    const auto& roads = map.GetRoads();

    std::uniform_int_distribution<> roads_dist(0, roads.size() - 1);
    const auto& road = roads[roads_dist(gen_)];
    const auto start_point = road.GetStart();
    if (road.IsVertical()) {
        std::uniform_real_distribution<> road_dist(
            static_cast<double>(start_point.y),
            static_cast<double>(road.GetEnd().y));
        return DogPosition{static_cast<double>(start_point.x), road_dist(gen_)};
    }
    std::uniform_real_distribution<> road_dist(
        static_cast<double>(start_point.x),
        static_cast<double>(road.GetEnd().x));
    return DogPosition{road_dist(gen_), static_cast<double>(start_point.y)};
}

DogPosition JoinGameUseCase::GetFirstRoadStartPos(const Map& map) const {
    const auto& roads = map.GetRoads();
    const auto& road = roads[0];
    const auto start_point = road.GetStart();

    return DogPosition{static_cast<double>(start_point.x),
                       static_cast<double>(start_point.y)};
}

DogState JoinGameUseCase::GetSpawnPoint(const Map& map) {
    switch (dog_spawn_mode_) {
        case DogSpawnMode::Random:
            return DogState{GetRandomPosition(map), DogSpeed{0, 0},
                            DogDirection::STOP};
        case DogSpawnMode::AtFirstRoadStart:
            return DogState{GetFirstRoadStartPos(map), DogSpeed{0, 0},
                            DogDirection::STOP};
        default:
            return DogState{GetFirstRoadStartPos(map), DogSpeed{0, 0},
                            DogDirection::STOP};
    }
}

json::value ListPlayersUseCase::GetPlayers(std::string_view auth_json) const {
    const auto* player = AuthorizePlayer(auth_json);
    const auto& dogs = player->GetSession()->GetDogs();

    json::object result;
    for (const auto& dog : dogs) {
        result[std::to_string(*dog->GetId())] =
            json::value{{"name"s, dog->GetName()}};
    }

    return result;
}

json::value GameStateUseCase::GetState(std::string_view auth_json) const {
    const auto* player = AuthorizePlayer(auth_json);

    json::object players_data;
    auto dogs = player->GetSession()->GetDogs();
    for (const auto& dog : dogs) {
        auto dog_state = dog->GetState();
        json::array dog_pos{dog_state.pos.x, dog_state.pos.y};
        json::array dog_speed{dog_state.speed.x, dog_state.speed.y};
        std::string dog_dir = DogDirToString(dog_state.dir);
        const auto dog_loot_bag = dog->GetLootBag();
        json::array loot_bag_json;
        for (const auto& loot : dog_loot_bag) {
            loot_bag_json.emplace_back(
                json::object{{"id"s, *loot->id_}, {"type"s, loot->type_}});
        }
        auto score = dog->GetScore();
        players_data[std::to_string(*dog->GetId())] =
            json::value{{"pos"s, std::move(dog_pos)},
                        {"speed"s, std::move(dog_speed)},
                        {"dir"s, std::move(dog_dir)},
                        {"bag"s, std::move(loot_bag_json)},
                        {"score"s, score}};
    }

    json::object loot_data;
    auto loot = player->GetSession()->GetLoot();
    for (const auto& loot_object : loot) {
        loot_data[std::to_string(*loot_object->id_)] =
            json::value{{"type"s, loot_object->type_},
                        {"pos"s, {loot_object->pos_.x, loot_object->pos_.y}}};
    }

    return json::value{{"players"s, std::move(players_data)},
                       {"lostObjects"s, std::move(loot_data)}};
}

std::string GameStateUseCase::DogDirToString(const DogDirection& dir) const {
    switch (dir) {
        case DogDirection::NORTH:
            return "U"s;
        case DogDirection::SOUTH:
            return "D"s;
        case DogDirection::WEST:
            return "L"s;
        case DogDirection::EAST:
            return "R"s;
        case DogDirection::STOP:
            return ""s;
        default:
            return ""s;
    }
}

void ActionUseCase::DoAction(std::string_view auth_json,
                             std::string_view action_json) const {
    const auto* player = AuthorizePlayer(auth_json);
    auto dir = ParseAction(action_json);
    player->GetSession()->MoveDog(player->GetDog()->GetId(), dir);
}

DogDirection ActionUseCase::StringToDogDir(std::string_view dir) const {
    if (dir == "U"sv) {
        return DogDirection::NORTH;
    }
    if (dir == "D"sv) {
        return DogDirection::SOUTH;
    }
    if (dir == "L"sv) {
        return DogDirection::WEST;
    }
    if (dir == "R"sv) {
        return DogDirection::EAST;
    }
    if (dir.empty()) {
        return DogDirection::STOP;
    }
    throw ApplicationException(ApplicationException::Error::Parse,
                               "wrong move direction"s);
}

DogDirection ActionUseCase::ParseAction(std::string_view action) const {
    auto action_obj = ParseJsonInput(action).get_object();

    auto move_it = action_obj.find("move");
    if (move_it == action_obj.end()) {
        throw ApplicationException(ApplicationException::Error::Parse,
                                   "move is not specified"s);
    }

    return StringToDogDir(move_it->value().get_string());
}

void TickUseCase::SkipTime(std::chrono::milliseconds time_delta) {
    game_.SkipTime(time_delta);
}

std::chrono::milliseconds TickUseCase::ParseTime(
    std::string_view time_delta_in_json) const {
    auto time_json_obj = ParseJsonInput(time_delta_in_json).get_object();

    auto time_it = time_json_obj.find("timeDelta");
    if (time_it == time_json_obj.end()) {
        throw ApplicationException(ApplicationException::Error::Parse,
                                   "time is not specified"s);
    }

    try {
        return std::chrono::milliseconds(time_it->value().as_int64());
    } catch (...) {
        throw ApplicationException(ApplicationException::Error::Parse,
                                   "wrong time number"s);
    }
}

void DBUseCase::SaveRecord(PlayerRecord&& record) {
    if (!factory_) {
        throw ApplicationException(ApplicationException::Error::DataBase,
                                   "сonnection factory not created"s);
    }
    auto uow = factory_->GetUnitOfWork();
    try {
        uow->Records().AddRecord(record);
        uow->Commit();
    } catch (const std::exception& exc) {
        uow->Rollback();
        throw exc;
    }
}
json::value DBUseCase::GetRecords(int start, int max_items) {
    if (max_items > default_max_items_output) {
        throw ApplicationException(ApplicationException::Error::DataBase,
                                   "max_items too big"s);
    }
    auto uow = factory_->GetUnitOfWork();
    return RecordsToJson(uow->Records().GetRecords(start, max_items));
}

json::value DBUseCase::RecordsToJson(std::vector<PlayerRecord> records) const {
    json::array result;
    result.reserve(records.size());
    for (const auto& record : records) {
        auto seconds =
            std::chrono::duration<double>(record.play_time_ms).count();
        result.emplace_back(json ::value{{"name", record.name},
                                         {"score", record.score},
                                         {"playTime", std::move(seconds)}});
    }
    return result;
}

PlayerRetirement::PlayerRetirement(Players& players,
                                   PlayerTokens& player_tokens, DBUseCase& db)
    : players_(players)
    , player_tokens_(player_tokens)
    , db_(db) {
}

void PlayerRetirement::Retire(std::shared_ptr<Dog>&& dog,
                              std::shared_ptr<GameSession>&& session) {
    db_.SaveRecord(
        PlayerRecord{dog->GetName(), dog->GetScore(), dog->GetPlayTime()});
    auto player =
        players_.FindByDogIdAndSessionId(*dog->GetId(), *session->GetId());
    player_tokens_.RemovePlayer(player);
    players_.Remove(std::move(dog), std::move(session));
}

Application::Application(Game& game, DogSpawnMode dog_spawn_mode,
                         TickerMode ticker_mode,
                         std::unique_ptr<UnitOfWorkFactory>&& factory)
    : game_(game)
    , dog_spawn_mode_(dog_spawn_mode)
    , ticker_mode_(ticker_mode)
    , maps_(game_, players_, player_tokens_)
    , join_game_(game_, players_, player_tokens_, dog_spawn_mode_)
    , list_players_(game_, players_, player_tokens_)
    , game_state_(game_, players_, player_tokens_)
    , action_(game_, players_, player_tokens_)
    , tick_(game_, players_, player_tokens_)
    , db_(std::move(factory))
    , player_retirement_(players_, player_tokens_, db_) {
    game_.SetRetirementObserver(
        std::make_shared<PlayerRetirement>(player_retirement_));
}

json::value Application::GetMaps() const {
    return maps_.GetMaps();
}

json::value Application::GetMap(std::string&& map_id) const {
    return maps_.GetMap(std::move(map_id));
}

json::value Application::JoinGame(std::string_view join_info_json) {
    return join_game_.Join(join_info_json);
}

json::value Application::GetPlayers(std::string_view auth_json) const {
    return list_players_.GetPlayers(auth_json);
}

json::value Application::GetState(std::string_view auth_json) const {
    return game_state_.GetState(auth_json);
}

void Application::DoAction(std::string_view auth_value,
                           std::string_view action) const {
    action_.DoAction(auth_value, action);
}

void Application::TickByRequest(std::string_view time_delta_in_json) {
    if (ticker_mode_ == TickerMode::ByRequest) {
        auto time_delta = tick_.ParseTime(time_delta_in_json);
        tick_.SkipTime(time_delta);
        if (listener_) {
            listener_->OnTick(time_delta);
        }
        return;
    }
    throw ApplicationException(ApplicationException::Error::Tick,
                               "App ticking in auto mode"s);
}

void Application::TickByTimer(std::chrono::milliseconds time_delta) {
    if (ticker_mode_ == TickerMode::Auto) {
        tick_.SkipTime(time_delta);
        if (listener_) {
            listener_->OnTick(time_delta);
        }
        return;
    }
    throw ApplicationException(ApplicationException::Error::Tick,
                               "App ticking in request mode"s);
}

void Application::SetListener(std::shared_ptr<ApplicationListener>&& listener) {
    listener_ = std::move(listener);
}
Players& Application::GetPlayers() {
    return players_;
}
const Players& Application::GetPlayers() const {
    return players_;
}

PlayerTokens& Application::GetPlayerTokens() {
    return player_tokens_;
}

const PlayerTokens& Application::GetPlayerTokens() const {
    return player_tokens_;
}

json::value Application::GetRecords(int start, int max_items) {
    return db_.GetRecords(start, max_items);
}
}  // namespace app