#include "model.h"

#include <stdexcept>

namespace model {

void Map::AddRoad(const Road& road) {
    roads_.emplace_back(road);
    UpdateGridSize(road);
}

void Map::AddBuilding(const Building& building) {
    buildings_.emplace_back(building);
}

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

void Map::BuildRoadsGrid() {
    grid_.resize(grid_size_y, std::vector<GridPoint>(grid_size_x));
    for (const auto& road : roads_) {
        if (road.IsHorizontal()) {
            for (auto x = road.GetMinX(); x <= road.GetMaxX(); ++x) {
                grid_[road.GetMinY()][x].h_road = &road;
            }
        } else {
            for (auto y = road.GetMinY(); y <= road.GetMaxY(); ++y) {
                grid_[y][road.GetMinX()].v_road = &road;
            }
        }
    }
}

void Map::SetDogSpeed(Speed speed) {
    dog_speed_ = speed;
}

Speed Map::GetDogSpeed() const {
    return dog_speed_;
}

void Map::SetLootBagCapacity(Capacity capacity) {
    lootbag_capacity_ = capacity;
}

Capacity Map::GetLootBagCapacity() const {
    return lootbag_capacity_;
}

void Map::SetDogRetirementTime(std::chrono::milliseconds time) {
    dog_retirement_time_ = time;
}
std::chrono::milliseconds Map::GetDogRetirementTime() const {
    return dog_retirement_time_;
}

GridPoint Map::GetGridPoint(Point point) const {
    return grid_.at(point.y).at(point.x);
}

const ExtraData& Map::GetExtraData() const {
    return extra_data_;
}

size_t Map::GetLootTypesNumber() const {
    return loot_type_to_value_.size();
}

size_t Map::GetLootValue(size_t loot_type) const {
    return loot_type_to_value_.at(loot_type);
}

void Map::UpdateGridSize(const Road& road) {
    grid_size_x = std::max(grid_size_x, road.GetMaxX() + 1);
    grid_size_y = std::max(grid_size_y, road.GetMaxY() + 1);
}

Dog::Id Dog::GetId() const noexcept {
    return id_;
}

std::string Dog::GetName() const noexcept {
    return name_;
}

DogState Dog::GetState() const noexcept {
    return state_;
}

void Dog::SetState(DogState state) {
    state_ = state;
}

bool Dog::IsMoving() const {
    return state_.dir != DogDirection::STOP;
}

Capacity Dog::GetLootBagCapacity() const {
    return lootbag_capacity_;
}

void Dog::TakeLoot(std::shared_ptr<LootObject>&& loot) {
    if (CanTakeLoot()) {
        lootbag_.push_back(std::move(loot));
    }
}

bool Dog::CanTakeLoot() const {
    return lootbag_.size() < lootbag_capacity_;
}

const Dog::LootBag& Dog::GetLootBag() const {
    return lootbag_;
}

std::vector<LootObject> Dog::GetLootBagContent() const {
    std::vector<LootObject> result;
    result.reserve(lootbag_.size());
    for (auto loot : lootbag_) {
        result.push_back(*loot);
    }
    return result;
}

void Dog::DeliverLoot() {
    for (auto loot : lootbag_) {
        score_ += loot->value_;
    }
    lootbag_.clear();
}

std::uint64_t Dog::GetScore() const noexcept {
    return score_;
}

void Dog::SetScore(std::uint64_t score) {
    score_ = score;
}

void Dog::UpdatePlayTime(std::chrono::milliseconds time_delta) {
    play_time_ += time_delta;
    if (IsMoving()) {
        inactive_time_ = 0ms;
    } else {
        inactive_time_ += time_delta;
    }
}

std::chrono::milliseconds Dog::GetPlayTime() const {
    return play_time_;
}
std::chrono::milliseconds Dog::GetInactiveTime() const {
    return inactive_time_;
}

DogState DogStateUpdater::MoveNorth(std::chrono::milliseconds time_delta,
                                    DogState dog_state) const {
    return MoveVertically(
        time_delta, dog_state,
        [](const Road& road) {
            return road.GetMinY() - distance_from_road_axis_;
        },
        [](double border, double new_y) { return std::max(border, new_y); });
}

DogState DogStateUpdater::MoveSouth(std::chrono::milliseconds time_delta,
                                    DogState dog_state) const {
    return MoveVertically(
        time_delta, dog_state,
        [](const Road& road) {
            return road.GetMaxY() + distance_from_road_axis_;
        },
        [](double border, double new_y) { return std::min(border, new_y); });
}

DogState DogStateUpdater::MoveWest(std::chrono::milliseconds time_delta,
                                   DogState dog_state) const {
    return MoveHorizontally(
        time_delta, dog_state,
        [](const Road& road) {
            return road.GetMinX() - distance_from_road_axis_;
        },
        [](double border, double new_x) { return std::max(border, new_x); });
}

DogState DogStateUpdater::MoveEast(std::chrono::milliseconds time_delta,
                                   DogState dog_state) const {
    return MoveHorizontally(
        time_delta, dog_state,
        [](const Road& road) {
            return road.GetMaxX() + distance_from_road_axis_;
        },
        [](double border, double new_x) { return std::min(border, new_x); });
}

Point DogStateUpdater::DogPosToPoint(DogPosition dog_pos) const {
    int x = static_cast<int>(std::round(dog_pos.x));
    int y = static_cast<int>(std::round(dog_pos.y));
    return Point{x, y};
}

size_t DogProvider::ItemsCount() const {
    return items_.size();
}
Item DogProvider::GetItem(size_t idx) const {
    return items_.at(idx);
}
size_t DogProvider::GatherersCount() const {
    return gatherers_.size();
}
Gatherer DogProvider::GetGatherer(size_t idx) const {
    return gatherers_.at(idx);
}

void DogProvider::AddGatherer(Gatherer&& gatherer) {
    gatherers_.push_back(std::move(gatherer));
}

void DogProvider::AddItem(const Item& item) {
    items_.push_back(item);
}

std::shared_ptr<Dog> DogProvider::GetDog(size_t idx) const {
    return dogs_.at(idx);
}

void DogProvider::AddDog(std::shared_ptr<Dog> dog) {
    dogs_.push_back(dog);
}

DogProvider::InteractionObject DogProvider::GetInteractionObject(
    size_t idx) const {
    return interaction_objects_.at(idx);
}

void DogProvider::AddInteractiobObject(InteractionObject object) {
    interaction_objects_.push_back(object);
}

const GameSession::Id& GameSession::GetId() const noexcept {
    return session_id_;
}

std::shared_ptr<GameSession> GameSession::GetShared() {
    return shared_from_this();
}

std::shared_ptr<Dog> GameSession::AddDog(std::string&& name,
                                         DogState&& spawn_point) {
    auto dog_id = Dog::Id{dog_indx_};
    auto emplaced = dogs_.emplace(
        dog_indx_, std::make_shared<Dog>(std::move(dog_id), std::move(name),
                                         std::move(spawn_point),
                                         map_->GetLootBagCapacity()));

    dog_indx_++;
    return emplaced.first->second;
}

void GameSession::AddDog(std::shared_ptr<Dog>&& dog) {
    dog_indx_ =
        (*dog->GetId() >= dog_indx_) ? (*dog->GetId() + 1) : (dog_indx_);
    dogs_[*dog->GetId()] = std::move(dog);
}

void GameSession::MoveDog(Dog::Id id, DogDirection dir) {
    DogSpeed speed;
    switch (dir) {
        case DogDirection::NORTH:
            speed = {0, -map_->GetDogSpeed()};
            break;
        case DogDirection::SOUTH:
            speed = {0, map_->GetDogSpeed()};
            break;
        case DogDirection::WEST:
            speed = {-map_->GetDogSpeed(), 0};
            break;
        case DogDirection::EAST:
            speed = {map_->GetDogSpeed(), 0};
            break;
        case DogDirection::STOP:
            speed = {0, 0};
            break;
        default:
            speed = {0, 0};
            break;
    }
    auto state = dogs_.at(*id)->GetState();
    state.speed = speed;
    state.dir = dir;
    dogs_.at(*id)->SetState(std::move(state));
}

void GameSession::AddLoot(std::shared_ptr<LootObject>&& loot) {
    loot_indx_ = (*loot->id_ >= loot_indx_) ? (*loot->id_ + 1) : (loot_indx_);
    loot_[*loot->id_] = std::move(loot);
}

const Map* GameSession::GetMap() const {
    return map_;
}

std::vector<std::shared_ptr<Dog>> GameSession::GetDogs() const {
    std::vector<std::shared_ptr<Dog>> result;
    result.reserve(dogs_.size());
    for (auto [id, dog] : dogs_) {
        result.push_back(dog);
    }
    return result;
}

std::vector<std::shared_ptr<LootObject>> GameSession::GetLoot() const {
    std::vector<std::shared_ptr<LootObject>> result;
    result.reserve(loot_.size());
    for (auto [id, loot] : loot_) {
        result.push_back(loot);
    }
    return result;
}

bool GameSession::IsAvailable() const {
    return true;
}

void GameSession::SkipTime(std::chrono::milliseconds time_delta) {
    if (time_delta == std::chrono::milliseconds::zero()) {
        return;
    }
    DogProvider provider;
    for (auto iter = dogs_.begin(); iter != dogs_.end();) {
        auto start_pos = geom::Point2D{iter->second->GetState().pos.x,
                                       iter->second->GetState().pos.y};
        UpdateDogState(time_delta, iter->second);
        iter->second->UpdatePlayTime(time_delta);
        if (DogShouldRetire(iter->second)) {
            RetireDog(iter->second);
            iter = dogs_.erase(iter);
            continue;
        }
        auto end_pos = geom::Point2D{iter->second->GetState().pos.x,
                                     iter->second->GetState().pos.y};
        provider.AddGatherer(Gatherer{start_pos, end_pos, Dog::DOG_GATHERING_WIDTH});
        provider.AddDog(iter->second);

        ++iter;
    }

    AddItemsToProvider(provider);
    auto events = FindGatherEvents(provider);
    HandleGatheringEvents(events, provider);

    GenerateLoot(time_delta);
}

void GameSession::UpdateDogState(std::chrono::milliseconds time_delta,
                                 std::shared_ptr<Dog> dog) const {
    if (!dog->IsMoving()) {
        return;
    }
    auto dog_state = dog->GetState();
    switch (dog_state.dir) {
        case DogDirection::NORTH:
            dog->SetState(dog_state_updater_.MoveNorth(time_delta, dog_state));
            break;
        case DogDirection::SOUTH:
            dog->SetState(dog_state_updater_.MoveSouth(time_delta, dog_state));
            break;
        case DogDirection::WEST:
            dog->SetState(dog_state_updater_.MoveWest(time_delta, dog_state));
            break;
        case DogDirection::EAST:
            dog->SetState(dog_state_updater_.MoveEast(time_delta, dog_state));
            break;
        case DogDirection::STOP:
            break;
        default:
            break;
    }
}

bool GameSession::DogShouldRetire(std::shared_ptr<Dog> dog) const {
    return dog->GetInactiveTime() >= map_->GetDogRetirementTime();
}

void GameSession::RetireDog(std::shared_ptr<Dog> dog) {
    auto dog_id = *dog->GetId();
    if (retirement_observer_) {
        retirement_observer_->Retire(std::move(dog), GetShared());
    }
}

LootPosition GameSession::GetRandomLootPosition() {
    const auto& roads = map_->GetRoads();

    std::uniform_int_distribution<> roads_dist(0, roads.size() - 1);
    const auto& road = roads.at(roads_dist(rand_gen_));
    const auto start_point = road.GetStart();
    if (road.IsVertical()) {
        std::uniform_real_distribution<> road_dist(
            static_cast<double>(start_point.y),
            static_cast<double>(road.GetEnd().y));
        return LootPosition{static_cast<double>(start_point.x),
                            road_dist(rand_gen_)};
    }
    std::uniform_real_distribution<> road_dist(
        static_cast<double>(start_point.x),
        static_cast<double>(road.GetEnd().x));
    return LootPosition{road_dist(rand_gen_),
                        static_cast<double>(start_point.y)};
}

void GameSession::AddLoot(size_t type, size_t value, LootPosition pos) {
    auto loot_id = LootObject::Id{loot_indx_};
    loot_.emplace(loot_indx_, std::make_shared<LootObject>(std::move(loot_id),
                                                           type, value, pos));
    loot_indx_++;
}

void GameSession::GenerateLoot(std::chrono::milliseconds time_delta) {
    auto amount = loot_gen_.Generate(time_delta, loot_.size(), dogs_.size());

    std::uniform_int_distribution<> loot_types_dist(
        0, map_->GetLootTypesNumber() - 1);
    for (size_t i = 0; i < amount; ++i) {
        auto loot_type = loot_types_dist(rand_gen_);
        AddLoot(loot_type, map_->GetLootValue(loot_type),
                GetRandomLootPosition());
    }
}

void GameSession::AddItemsToProvider(DogProvider& provider) const {
    for (auto [id, loot] : loot_) {
        provider.AddItem(
            Item{geom::Point2D{loot->pos_.x, loot->pos_.y}, LootObject::WIDTH});
        provider.AddInteractiobObject(loot);
    }

    for (const auto& office : map_->GetOffices()) {
        provider.AddItem(
            Item{geom::Point2D{static_cast<double>(office.GetPosition().x),
                               static_cast<double>(office.GetPosition().y)},
                 Office::WIDTH});
        provider.AddInteractiobObject(&office);
    }
}

void GameSession::HandleGatheringEvents(
    const std::vector<GatheringEvent>& events, const DogProvider& provider) {
    for (const auto& event : events) {
        auto dog = provider.GetDog(event.gatherer_id);
        auto object = provider.GetInteractionObject(event.item_id);
        if (std::holds_alternative<const Office*>(object)) {
            dog->DeliverLoot();
        } else {
            auto loot_id =
                *std::get<const std::shared_ptr<LootObject>>(object)->id_;
            if (!dog->CanTakeLoot()) {
                return;
            }
            if (auto node = loot_.extract(loot_id); !node.empty()) {
                dog->TakeLoot(std::move(node.mapped()));
            }
        }
    }
}

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index);
        !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() +
                                    " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

const Map* Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

std::shared_ptr<GameSession> Game::GetSessionByMapPtr(const Map* map_ptr) {
    if (auto it = map_ptr_to_sessions_id_.find(map_ptr);
        it != map_ptr_to_sessions_id_.end()) {
        for (auto& id : it->second) {
            if (sessions_.at(id) && sessions_.at(id)->IsAvailable()) {
                return sessions_.at(id);
            }
        }
    }
    return CreateGameSession(map_ptr);
}

std::shared_ptr<GameSession> Game::GetSessionByIdAndMapPtr(
    const GameSession::Id& id, const Map* map_ptr) {
    if (auto it = sessions_.find(id); it != sessions_.end()) {
        return it->second;
    }

    return CreateSessionByIdAndMapPtr(id, map_ptr);
}

const LootGen& Game::GetLootGenerator() const {
    return loot_gen_;
}

void Game::AddSession(std::shared_ptr<GameSession>&& session) {
    session_indx_ = (*session->GetId() >= session_indx_)
                        ? (*session->GetId() + 1)
                        : (session_indx_);

    map_ptr_to_sessions_id_[session->GetMap()].insert(session->GetId());
    sessions_[session->GetId()] = std::move(session);
}

std::vector<GameSession> Game::GetSessions() const {
    std::vector<GameSession> result;
    result.reserve(sessions_.size());
    for (const auto& [id, session_ptr] : sessions_) {
        result.push_back(*session_ptr);
    }

    return result;
}

void Game::SkipTime(std::chrono::milliseconds time_delta) {
    for (const auto& session : sessions_) {
        session.second->SkipTime(time_delta);
    }
}

void Game::SetRetirementObserver(
    std::shared_ptr<RetirementObserver>&& observer) {
    observer_ = std::move(observer);
}

std::shared_ptr<RetirementObserver> Game::GetRetirementObserver() const {
    return observer_;
}

std::shared_ptr<GameSession> Game::CreateGameSession(const Map* map_ptr) {
    auto session_id = GameSession::Id{session_indx_++};
    sessions_[session_id] = std::make_shared<GameSession>(session_id, map_ptr,
                                                          loot_gen_, observer_);
    map_ptr_to_sessions_id_[map_ptr].insert(session_id);

    return sessions_.at(session_id);
}

std::shared_ptr<GameSession> Game::CreateSessionByIdAndMapPtr(
    const GameSession::Id& id, const Map* map_ptr) {
    sessions_[id] =
        std::make_shared<GameSession>(id, map_ptr, loot_gen_, observer_);
    map_ptr_to_sessions_id_[map_ptr].insert(id);

    session_indx_ = (*id >= session_indx_) ? (*id + 1) : (session_indx_);
    return sessions_.at(id);
}

}  // namespace model