#pragma once

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "app.h"
#include "model.h"

namespace model {

template <typename Archive>
void serialize(Archive& ar, DogPosition& pos,
               [[maybe_unused]] const unsigned version) {
    ar & pos.x;
    ar & pos.y;
}

template <typename Archive>
void serialize(Archive& ar, DogSpeed& speed,
               [[maybe_unused]] const unsigned version) {
    ar & speed.x;
    ar & speed.y;
}

template <typename Archive>
void serialize(Archive& ar, DogState& state,
               [[maybe_unused]] const unsigned version) {
    ar & state.pos;
    ar & state.speed;
    ar & state.dir;
}

template <typename Archive>
void serialize(Archive& ar, LootPosition& pos,
               [[maybe_unused]] const unsigned version) {
    ar & pos.x;
    ar & pos.y;
}

template <typename Archive>
void serialize(Archive& ar, LootObject& obj,
               [[maybe_unused]] const unsigned version) {
    ar&(*obj.id_);
    ar & obj.type_;
    ar & obj.value_;
    ar & obj.pos_;
}

}  // namespace model

namespace serialization {

using namespace std::literals;
using namespace app;
using namespace model;

// DogRepr (DogRepresentation) - сериализованное представление класса Dog
class DogRepr {
public:
    DogRepr() = default;
    explicit DogRepr(const model::Dog& dog);
    [[nodiscard]] Dog Restore() const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar&(*id_);
        ar & name_;
        ar & state_;
        ar & lootbag_capacity_;
        ar & lootbag_;
        ar & score_;
    }

private:
    model::Dog::Id id_ = model::Dog::Id{0ul};
    std::string name_;
    model::DogState state_;

    model::Capacity lootbag_capacity_;
    std::vector<model::LootObject> lootbag_;
    std::uint64_t score_ = 0;
};

// GameSessionRepr - сериализованное представление класса Game
class GameSessionRepr {
public:
    GameSessionRepr() = default;
    explicit GameSessionRepr(const GameSession& session);
    [[nodiscard]] GameSession Restore(const Game& game) const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & session_id_;
        ar & map_id_;
        ar & dogs_repr_;
        ar & loot_on_map_;
    }

private:
    void FillDogsRepr(const GameSession& session);
    void FillLootOnMap(const GameSession& session);

private:
    std::uint64_t session_id_;
    std::string map_id_;

    std::vector<DogRepr> dogs_repr_;
    std::vector<LootObject> loot_on_map_;
};

// ApplicationRepr - сериализованное представление приложения
class ApplicationRepr {
public:
    ApplicationRepr() = default;
    explicit ApplicationRepr(Game& game, Application& app);
    struct DogSessionId {
        bool operator==(const DogSessionId& other) const {
            return dog_id == other.dog_id && session_id == other.session_id;
        }

        template <typename Archive>
        void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
            ar & dog_id;
            ar & session_id;
        }

        std::uint64_t dog_id = 0;
        std::uint64_t session_id = 0;
    };
    void Restore(Game& game, Application& app) const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & sessions_repr_;
        ar & dog_session_id_to_tokens_;
    }

private:
    void FillSessionsRepr(Game& game);
    void FillPlayersToTokensMap(Application& app);

private:
    std::vector<GameSessionRepr> sessions_repr_;

    struct IdHasher {
        size_t operator()(const DogSessionId& key) const {
            return std::hash<uint64_t>{}(key.dog_id) ^
                   (std::hash<uint64_t>{}(key.session_id) << 1);
        }
    };

    std::unordered_map<DogSessionId, std::string, IdHasher>
        dog_session_id_to_tokens_;
};

namespace fs = std::filesystem;
namespace chrono = std::chrono;

class SerializingListener : public app::ApplicationListener {
public:
    SerializingListener(Game& game, Application& app,
                        const fs::path& state_file);

    void OnTick(chrono::milliseconds time_delta) override;
    void SetSavePeriod(chrono::milliseconds period);

    void SaveState();
    void RestoreState();

private:
    Application& app_;
    Game& game_;
    fs::path state_file_;

    std::optional<chrono::milliseconds> save_period_;
    chrono::milliseconds time_since_save_;
};

}  // namespace serialization
