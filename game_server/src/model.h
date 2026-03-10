#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "collision_detector.h"
#include "data_keeper.h"
#include "geom.h"
#include "loot_generator.h"
#include "tagged.h"
#include "utils.h"

namespace model {
using namespace std::literals;

using Dimension = int;
using Coord = Dimension;

using Speed = double;

using Capacity = size_t;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

struct DogPosition {
    auto operator<=>(const DogPosition&) const = default;

    double x, y;
};

struct DogSpeed {
    bool IsZero() const noexcept {
        return x == 0 && y == 0;
    }

    auto operator<=>(const DogSpeed&) const = default;

    Speed x = 0.0, y = 0.0;
};

enum class DogDirection {
    NORTH,
    SOUTH,
    WEST,
    EAST,
    STOP
};

struct DogState {
    auto operator<=>(const DogState&) const = default;

    DogPosition pos;
    DogSpeed speed;
    DogDirection dir;
};

class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

    Coord GetMaxX() const {
        return std::max(start_.x, end_.x);
    }

    Coord GetMinX() const {
        return std::min(start_.x, end_.x);
    }

    Coord GetMaxY() const {
        return std::max(start_.y, end_.y);
    }

    Coord GetMinY() const {
        return std::min(start_.y, end_.y);
    }

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    constexpr static double WIDTH = 0.5;
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

struct GridPoint {
    const Road* h_road = nullptr;
    const Road* v_road = nullptr;

    bool IsOnHorizontalRoad() {
        return h_road != nullptr;
    }

    bool IsOnVerticalRoad() {
        return v_road != nullptr;
    }
};

using namespace data_keeper;

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name,
        std::unordered_map<size_t, size_t>&& loot_type_to_value,
        ExtraData extra_data) noexcept
        : id_(std::move(id))
        , name_(std::move(name))
        , loot_type_to_value_(std::move(loot_type_to_value))
        , extra_data_(std::move(extra_data)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddRoad(const Road& road);
    void AddBuilding(const Building& building);
    void AddOffice(Office office);

    void BuildRoadsGrid();
    GridPoint GetGridPoint(Point point) const;

    void SetDogSpeed(Speed speed);
    Speed GetDogSpeed() const;

    void SetLootBagCapacity(Capacity capacity);
    Capacity GetLootBagCapacity() const;

    void SetDogRetirementTime(std::chrono::milliseconds time);
    std::chrono::milliseconds GetDogRetirementTime() const;

    const ExtraData& GetExtraData() const;

    size_t GetLootTypesNumber() const;
    size_t GetLootValue(size_t loot_type) const;

private:
    void UpdateGridSize(const Road& road);

private:
    using OfficeIdToIndex =
        std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    std::unordered_map<size_t, size_t> loot_type_to_value_;

    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;

    Speed dog_speed_ = 1;
    Capacity lootbag_capacity_ = 3;
    std::chrono::milliseconds dog_retirement_time_{0ms};

    int grid_size_x = 0;
    int grid_size_y = 0;
    using Grid = std::vector<std::vector<GridPoint>>;
    Grid grid_;

    ExtraData extra_data_;
};

struct LootPosition {
    auto operator<=>(const LootPosition&) const = default;

    double x, y;
};

struct LootObject {
    constexpr static double WIDTH = 0.0;
    using Id = util::Tagged<std::uint64_t, LootObject>;

    LootObject() = default;
    LootObject(Id&& id, size_t type, size_t value, LootPosition pos)
        : id_(std::move(id))
        , type_(type)
        , value_(value)
        , pos_(pos) {
    }

    auto operator<=>(const LootObject&) const = default;

    Id id_ = Id{0ul};
    size_t type_;
    size_t value_;
    LootPosition pos_;
};

class Dog {
public:
    constexpr static double DOG_GATHERING_WIDTH = 0.6;
    using Id = util::Tagged<std::uint64_t, Dog>;

    Dog(Id&& id, std::string&& name, DogState&& state,
        Capacity lootbag_capacity) noexcept
        : id_(std::move(id))
        , name_(std::move(name))
        , state_(std::move(state))
        , lootbag_capacity_(lootbag_capacity) {
    }

    Dog(const Id& id, const std::string& name, const DogState& state,
        Capacity lootbag_capacity) noexcept
        : id_(id)
        , name_(name)
        , state_(state)
        , lootbag_capacity_(lootbag_capacity) {
    }

    Id GetId() const noexcept;
    std::string GetName() const noexcept;

    DogState GetState() const noexcept;
    void SetState(DogState state);

    bool IsMoving() const;

    Capacity GetLootBagCapacity() const;
    void TakeLoot(std::shared_ptr<LootObject>&& loot);
    bool CanTakeLoot() const;

    using LootBag = std::vector<std::shared_ptr<LootObject>>;
    const LootBag& GetLootBag() const;
    std::vector<LootObject> GetLootBagContent() const;
    void DeliverLoot();

    std::uint64_t GetScore() const noexcept;
    void SetScore(std::uint64_t score);

    void UpdatePlayTime(std::chrono::milliseconds time_delta);
    std::chrono::milliseconds GetPlayTime() const;
    std::chrono::milliseconds GetInactiveTime() const;

private:
    Id id_;
    std::string name_;
    DogState state_;

    Capacity lootbag_capacity_;
    LootBag lootbag_;
    std::uint64_t score_ = 0;

    std::chrono::milliseconds play_time_{0ms};
    std::chrono::milliseconds inactive_time_{0ms};
};

class DogStateUpdater {
public:
    DogStateUpdater(const Map* map)
        : map_(map) {
    }

    DogState MoveNorth(std::chrono::milliseconds time_delta,
                       DogState dog_state) const;
    DogState MoveSouth(std::chrono::milliseconds time_delta,
                       DogState dog_state) const;
    DogState MoveWest(std::chrono::milliseconds time_delta,
                      DogState dog_state) const;
    DogState MoveEast(std::chrono::milliseconds time_delta,
                      DogState dog_state) const;

private:
    Point DogPosToPoint(DogPosition dog_pos) const;

    template <typename BorderGetter, typename BorderComparator>
    DogState MoveVertically(std::chrono::milliseconds time_delta,
                            DogState dog_state, BorderGetter&& get_road_border,
                            BorderComparator&& compare_with_road_border) const {
        DogPosition dog_start_pos = dog_state.pos;
        GridPoint start_grid_point =
            map_->GetGridPoint(DogPosToPoint(dog_start_pos));

        double road_border;
        if (start_grid_point.IsOnVerticalRoad()) {
            road_border = get_road_border(*start_grid_point.v_road);
        } else {
            road_border = get_road_border(*start_grid_point.h_road);
        }

        double new_dog_y =
            dog_start_pos.y +
            utils::CalculateDistanceInUnits(
                dog_state.speed.y, static_cast<double>(time_delta.count()));

        new_dog_y = compare_with_road_border(road_border, new_dog_y);
        bool ran_into_border = (new_dog_y == road_border);

        DogPosition new_dog_pos{dog_start_pos.x, new_dog_y};
        DogSpeed new_dog_speed =
            ran_into_border ? DogSpeed{0, 0} : dog_state.speed;

        return DogState(new_dog_pos, new_dog_speed, dog_state.dir);
    }

    template <typename BorderGetter, typename BorderComparator>
    DogState MoveHorizontally(
        std::chrono::milliseconds time_delta, DogState dog_state,
        BorderGetter&& get_road_border,
        BorderComparator&& compare_with_road_border) const {
        DogPosition dog_start_pos = dog_state.pos;
        GridPoint start_grid_point =
            map_->GetGridPoint(DogPosToPoint(dog_start_pos));

        double road_border;
        if (start_grid_point.IsOnHorizontalRoad()) {
            road_border = get_road_border(*start_grid_point.h_road);
        } else {
            road_border = get_road_border(*start_grid_point.v_road);
        }

        double new_dog_x =
            dog_start_pos.x +
            utils::CalculateDistanceInUnits(
                dog_state.speed.x, static_cast<double>(time_delta.count()));

        new_dog_x = compare_with_road_border(road_border, new_dog_x);
        bool ran_into_border = (new_dog_x == road_border);

        DogPosition new_dog_pos{new_dog_x, dog_start_pos.y};
        DogSpeed new_dog_speed =
            ran_into_border ? DogSpeed{0, 0} : dog_state.speed;

        return DogState(new_dog_pos, new_dog_speed, dog_state.dir);
    }

private:
    static constexpr double distance_from_road_axis_ = 0.4;
    const Map* map_;
};

using namespace collision_detector;

class DogProvider : public ItemGathererProvider {
public:
    size_t ItemsCount() const override;
    Item GetItem(size_t idx) const override;
    size_t GatherersCount() const override;
    Gatherer GetGatherer(size_t idx) const override;

    void AddGatherer(Gatherer&& gatherer);
    void AddItem(const Item& item);

    std::shared_ptr<Dog> GetDog(size_t idx) const;
    void AddDog(std::shared_ptr<Dog> dog);

    using InteractionObject =
        std::variant<const std::shared_ptr<LootObject>, const Office*>;
    InteractionObject GetInteractionObject(size_t idx) const;

    void AddInteractiobObject(InteractionObject object);

private:
    std::vector<Gatherer> gatherers_;
    std::vector<Item> items_;

    std::vector<std::shared_ptr<Dog>> dogs_;
    std::vector<InteractionObject> interaction_objects_;
};
class GameSession;

class RetirementObserver {
public:
    virtual void Retire(std::shared_ptr<Dog>&& dog,
                        std::shared_ptr<GameSession>&& session) = 0;
};

using LootGen = loot_gen::LootGenerator;

class GameSession : public std::enable_shared_from_this<GameSession> {
public:
    using Id = util::Tagged<std::uint64_t, GameSession>;

    GameSession(Id id, const Map* map, const LootGen& loot_gen,
                std::shared_ptr<RetirementObserver> retirement_observer)
        : session_id_(std::move(id))
        , map_(map)
        , dog_state_updater_(map)
        , loot_gen_(loot_gen)
        , rand_gen_(std::random_device{}())
        , retirement_observer_(std::move(retirement_observer)) {
    }

    std::shared_ptr<GameSession> GetShared();

    const Id& GetId() const noexcept;
    const Map* GetMap() const;
    std::vector<std::shared_ptr<Dog>> GetDogs() const;
    std::vector<std::shared_ptr<LootObject>> GetLoot() const;

    std::shared_ptr<Dog> AddDog(std::string&& name, DogState&& spawn_point);
    void MoveDog(Dog::Id id, DogDirection dir);

    void AddDog(std::shared_ptr<Dog>&& dog);
    void AddLoot(std::shared_ptr<LootObject>&& loot);

    void SkipTime(std::chrono::milliseconds time_delta);
    // На одной карте может быть несколько сессий, поэтому осталась заглушка для
    // условия доступности сессии
    bool IsAvailable() const;

private:
    void UpdateDogState(std::chrono::milliseconds time_delta,
                        std::shared_ptr<Dog> dog) const;
    bool DogShouldRetire(std::shared_ptr<Dog> dog) const;
    void RetireDog(std::shared_ptr<Dog> dog);
    LootPosition GetRandomLootPosition();
    void AddLoot(size_t type, size_t value, LootPosition pos);
    void GenerateLoot(std::chrono::milliseconds time_delta);
    void AddItemsToProvider(DogProvider& provider) const;
    void HandleGatheringEvents(const std::vector<GatheringEvent>& events,
                               const DogProvider& provider);

private:
    Id session_id_;
    const Map* map_;
    DogStateUpdater dog_state_updater_;
    LootGen loot_gen_;
    std::mt19937 rand_gen_;
    std::shared_ptr<RetirementObserver> retirement_observer_ = nullptr;

    std::uint64_t dog_indx_ = 0;
    std::unordered_map<std::uint64_t, std::shared_ptr<Dog>> dogs_;

    std::uint64_t loot_indx_ = 0;
    std::unordered_map<std::uint64_t, std::shared_ptr<LootObject>> loot_;
};

class Game {
public:
    using Maps = std::vector<Map>;

    Game(LootGen&& loot_gen)
        : loot_gen_(std::move(loot_gen)) {
    }

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }
    const Map* FindMap(const Map::Id& id) const noexcept;

    std::shared_ptr<GameSession> GetSessionByMapPtr(const Map* map_ptr);
    std::shared_ptr<GameSession> GetSessionByIdAndMapPtr(
        const GameSession::Id& id, const Map* map_ptr);

    const LootGen& GetLootGenerator() const;

    void AddSession(std::shared_ptr<GameSession>&& session);

    std::vector<GameSession> GetSessions() const;
    void SkipTime(std::chrono::milliseconds time_delta);

    void SetRetirementObserver(std::shared_ptr<RetirementObserver>&& observer);
    std::shared_ptr<RetirementObserver> GetRetirementObserver() const;

private:
    std::shared_ptr<GameSession> CreateGameSession(const Map* map_ptr);
    std::shared_ptr<GameSession> CreateSessionByIdAndMapPtr(
        const GameSession::Id& id, const Map* map_ptr);

private:
    LootGen loot_gen_;
    std::shared_ptr<RetirementObserver> observer_;
    
    std::vector<Map> maps_;
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    MapIdToIndex map_id_to_index_;

    using GameSessionIdHasher = util::TaggedHasher<GameSession::Id>;

    std::uint64_t session_indx_ = 0;
    std::unordered_map<GameSession::Id, std::shared_ptr<GameSession>,
                       GameSessionIdHasher>
        sessions_;
    std::unordered_map<const Map*,
                       std::unordered_set<GameSession::Id, GameSessionIdHasher>>
        map_ptr_to_sessions_id_;
};

}  // namespace model
