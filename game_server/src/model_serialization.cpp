#include "model_serialization.h"

namespace serialization {
DogRepr::DogRepr(const model::Dog& dog)
    : id_(dog.GetId())
    , name_(dog.GetName())
    , state_(dog.GetState())
    , lootbag_capacity_(dog.GetLootBagCapacity())
    , lootbag_(dog.GetLootBagContent())
    , score_(dog.GetScore()) {
}

Dog DogRepr::Restore() const {
    Dog dog{id_, name_, state_, lootbag_capacity_};
    dog.SetScore(score_);
    for (auto item : lootbag_) {
        if (!dog.CanTakeLoot()) {
            throw std::runtime_error("Failed to put bag content");
        }
        dog.TakeLoot(std::make_shared<model::LootObject>(item));
    }
    return dog;
}

GameSessionRepr::GameSessionRepr(const GameSession& session)
    : session_id_(*session.GetId())
    , map_id_(*session.GetMap()->GetId()) {
    FillDogsRepr(session);
    FillLootOnMap(session);
}

GameSession GameSessionRepr::Restore(const Game& game) const {
    GameSession session{GameSession::Id{session_id_},
                        game.FindMap(Map::Id{map_id_}), game.GetLootGenerator(),
                        game.GetRetirementObserver()};

    for (const auto& dog : dogs_repr_) {
        session.AddDog(std::make_shared<Dog>(dog.Restore()));
    }
    for (const auto& loot : loot_on_map_) {
        session.AddLoot(std::make_shared<LootObject>(loot));
    }
    return session;
}

void GameSessionRepr::FillDogsRepr(const GameSession& session) {
    auto dogs = session.GetDogs();
    dogs_repr_.reserve(dogs.size());
    for (const auto& dog : dogs) {
        dogs_repr_.emplace_back(DogRepr{*dog});
    }
}

void GameSessionRepr::FillLootOnMap(const GameSession& session) {
    auto loot = session.GetLoot();
    loot_on_map_.reserve(loot.size());
    for (const auto& loot_obj : loot) {
        loot_on_map_.push_back(*loot_obj);
    }
}

ApplicationRepr::ApplicationRepr(Game& game, Application& app) {
    FillSessionsRepr(game);
    FillPlayersToTokensMap(app);
}

void ApplicationRepr::Restore(Game& game, Application& app) const {
    for (const auto& session_repr : sessions_repr_) {
        auto session =
            std::make_shared<GameSession>(session_repr.Restore(game));
        for (auto dog : session->GetDogs()) {
            DogSessionId dog_session_id{*dog->GetId(), *session->GetId()};
            auto token = dog_session_id_to_tokens_.at(dog_session_id);
            auto& player = app.GetPlayers().Add(
                std::move(dog), std::move(session->GetShared()));
            app.GetPlayerTokens().AddPlayerWithToken(player, Token{token});
        }
        game.AddSession(std::move(session));
    }
}

void ApplicationRepr::FillSessionsRepr(Game& game) {
    auto sessions = game.GetSessions();
    sessions_repr_.reserve(sessions.size());
    for (const auto& session : sessions) {
        sessions_repr_.emplace_back(GameSessionRepr{session});
    }
}

void ApplicationRepr::FillPlayersToTokensMap(Application& app) {
    const auto& tokens_to_players = app.GetPlayerTokens().GetTokensToPlayers();
    for (const auto& [token, player] : tokens_to_players) {
        DogSessionId dog_session_id{*player->GetId(), *player->GetSessionId()};
        dog_session_id_to_tokens_[std::move(dog_session_id)] = *token;
    }
}

SerializingListener::SerializingListener(Game& game, Application& app,
                                         const fs::path& state_file)
    : game_(game)
    , app_(app)
    , state_file_(state_file)
    , save_period_(std::nullopt)
    , time_since_save_{0ms} {
}

void SerializingListener::OnTick(std::chrono::milliseconds time_delta) {
    if (save_period_) {
        time_since_save_ += time_delta;
        if (time_since_save_ >= save_period_.value()) {
            SaveState();
            time_since_save_ = 0ms;
        }
    }
}

void SerializingListener::SetSavePeriod(chrono::milliseconds period) {
    save_period_ = period;
}

void SerializingListener::SaveState() {
    fs::path temporary_file = state_file_.parent_path() / "temp_game_state"s;
    std::ofstream out_file{temporary_file, std::ios::out | std::ios::app};
    if (!out_file.is_open()) {
        throw std::runtime_error("Can't create new state file"s);
    }

    boost::archive::text_oarchive out_ar{out_file};
    ApplicationRepr repr{game_, app_};
    out_ar << repr;

    std::filesystem::rename(temporary_file, state_file_);
}

void SerializingListener::RestoreState() {
    if (!fs::exists(state_file_)) {
        return;
    }
    std::ifstream in_file{state_file_, std::ios::in};
    if (!in_file.is_open()) {
        throw std::runtime_error("Can't open state file"s);
    }

    boost::archive::text_iarchive in_ar{in_file};
    ApplicationRepr repr;
    in_ar >> repr;
    repr.Restore(game_, app_);
}

}  // namespace serialization