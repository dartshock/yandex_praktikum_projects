#pragma once

// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <chrono>
#include <iomanip>
#include <memory>
#include <pqxx/connection>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

#include "connection_pool.h"
#include "model.h"
#include "tagged.h"

namespace app {

namespace json = boost::json;
namespace sys = boost::system;

using namespace model;
using namespace std::literals;

class Player {
public:
    using Id = util::Tagged<std::uint64_t, Player>;

    Player(std::shared_ptr<Dog>&& dog, std::shared_ptr<GameSession>&& session)
        : id_(Player::Id{*dog->GetId()})
        , dog_(std::move(dog))
        , session_(std::move(session)) {
    }

    Id GetId() const noexcept {
        return id_;
    }

    GameSession::Id GetSessionId() const noexcept {
        return session_->GetId();
    }

    std::shared_ptr<GameSession> GetSession() const {
        return session_->GetShared();
    }

    std::shared_ptr<Dog> GetDog() const {
        return dog_;
    }

private:
    Id id_;
    std::shared_ptr<Dog> dog_;
    std::shared_ptr<GameSession> session_;
};

struct PlayerRecord {
    std::string name;
    std::uint64_t score;
    std::chrono::milliseconds play_time_ms;
};

class RecordsRepository {
public:
    virtual ~RecordsRepository() = default;
    virtual void AddRecord(const PlayerRecord& record) = 0;
    virtual std::vector<PlayerRecord> GetRecords(int start, int max_items) = 0;
};

class Players {
public:
    Player& Add(std::shared_ptr<Dog>&& dog,
                std::shared_ptr<GameSession>&& session);
    void Remove(std::shared_ptr<Dog>&& dog,
                std::shared_ptr<GameSession>&& session);
    const Player* FindByDogIdAndSessionId(std::uint64_t dog_id,
                                          std::uint64_t session_id);

private:
    struct CompositeKey {
        std::uint64_t dog_id;
        std::uint64_t session_id;

        bool operator==(const CompositeKey& other) const {
            return dog_id == other.dog_id && session_id == other.session_id;
        }
    };

    struct KeyHasher {
        size_t operator()(const CompositeKey& key) const {
            return std::hash<uint64_t>{}(key.dog_id) ^
                   (std::hash<uint64_t>{}(key.session_id) << 1);
        }
    };

    std::unordered_map<CompositeKey, Player, KeyHasher> players_;
};

namespace detail {
struct TokenTag {};
}  // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;

class PlayerTokens {
public:
    PlayerTokens()
        : gen_(std::random_device{}()) {
    }

    const Player* FindPlayerByToken(const Token& token);
    Token AddPlayer(Player& player);
    void AddPlayerWithToken(Player& player, const Token& token);
    void RemovePlayer(const Player* player);
    Token GenerateToken();

    using TokenHasher = util::TaggedHasher<Token>;
    using TokensToPlayers =
        std::unordered_map<Token, const Player*, TokenHasher>;
    const TokensToPlayers& GetTokensToPlayers() const;

private:
    std::mt19937_64 gen_;
    TokensToPlayers token_to_player_;
    std::unordered_map<const Player*, Token> player_to_token_;
};

enum class DogSpawnMode {
    Random,
    AtFirstRoadStart
};

enum class TickerMode {
    Auto,
    ByRequest
};

class ApplicationException {
public:
    enum class Error {
        Parse,
        InvalidToken,
        UnknownToken,
        UnknownMap,
        Tick,
        DataBase
    };

    ApplicationException(Error err, const std::string& msg)
        : error_(err)
        , message_(msg) {
    }

    Error GetError() const {
        return error_;
    }

    bool operator==(ApplicationException rhs) const {
        return error_ == rhs.error_;
    }

    std::string ToString() const;

private:
    Error error_;
    std::string message_;
};

class ApplicationUseCase {
public:
    ApplicationUseCase(Game& game, Players& players, PlayerTokens& tokens)
        : game_(game)
        , players_(players)
        , player_tokens_(tokens) {
    }

    virtual ~ApplicationUseCase() = default;

protected:
    Token GetPlayerToken(std::string_view auth_value) const;
    const Player* AuthorizePlayer(std::string_view auth_value) const;
    const Map* GetMapPtrByMapId(std::string&& map_id) const;
    std::shared_ptr<GameSession> GetSessionByMapId(std::string&& map_id) const;
    json::value ParseJsonInput(std::string_view input) const;

protected:
    Game& game_;
    Players& players_;
    PlayerTokens& player_tokens_;
};

class MapsUseCase : public ApplicationUseCase {
public:
    MapsUseCase(Game& game, Players& players, PlayerTokens& tokens)
        : ApplicationUseCase(game, players, tokens) {
    }

    json::value GetMaps() const;
    json::value GetMap(std::string&& map_id) const;

private:
    template <typename Container, typename ToJsonFunc>
    json::array ContainerToJson(const Container& container,
                                ToJsonFunc to_json) const {
        json::array result;
        result.reserve(std::size(container));
        for (const auto& item : container) {
            result.push_back(to_json(item));
        }
        return result;
    }
};

class JoinGameUseCase : public ApplicationUseCase {
public:
    JoinGameUseCase(Game& game, Players& players, PlayerTokens& tokens,
                    DogSpawnMode dog_spawn_mode)
        : ApplicationUseCase(game, players, tokens)
        , dog_spawn_mode_(dog_spawn_mode)
        , gen_(std::random_device{}()) {
    }

    json::value Join(std::string_view join_info_json);

private:
    json::value AddPlayerOnMap(std::string&& username, std::string&& map_id);
    DogPosition GetRandomPosition(const Map& map);
    DogPosition GetFirstRoadStartPos(const Map& map) const;
    DogState GetSpawnPoint(const Map& map);

private:
    const DogSpawnMode dog_spawn_mode_;
    std::mt19937 gen_;
};

class ListPlayersUseCase : public ApplicationUseCase {
public:
    ListPlayersUseCase(Game& game, Players& players, PlayerTokens& tokens)
        : ApplicationUseCase(game, players, tokens) {
    }

    json::value GetPlayers(std::string_view auth_json) const;
};

class GameStateUseCase : public ApplicationUseCase {
public:
    GameStateUseCase(Game& game, Players& players, PlayerTokens& tokens)
        : ApplicationUseCase(game, players, tokens) {
    }

    json::value GetState(std::string_view auth_json) const;

private:
    std::string DogDirToString(const DogDirection& dir) const;
};

class ActionUseCase : public ApplicationUseCase {
public:
    ActionUseCase(Game& game, Players& players, PlayerTokens& tokens)
        : ApplicationUseCase(game, players, tokens) {
    }

    void DoAction(std::string_view auth_json,
                  std::string_view action_json) const;

private:
    DogDirection StringToDogDir(std::string_view dir) const;
    DogDirection ParseAction(std::string_view action) const;
};

class TickUseCase : public ApplicationUseCase {
public:
    TickUseCase(Game& game, Players& players, PlayerTokens& tokens)
        : ApplicationUseCase(game, players, tokens) {
    }

    std::chrono::milliseconds ParseTime(
        std::string_view time_delta_in_json) const;
    void SkipTime(std::chrono::milliseconds time_delta);
};

class UnitOfWork {
public:
    virtual ~UnitOfWork() = default;
    virtual RecordsRepository& Records() = 0;
    virtual void Commit() = 0;
    virtual void Rollback() = 0;
};

class UnitOfWorkFactory {
public:
    virtual ~UnitOfWorkFactory() = default;
    virtual std::unique_ptr<app::UnitOfWork> GetUnitOfWork() = 0;
};

struct OutputParams {
    int start = 0;
    int maxItems = 0;
};

class DBUseCase {
public:
    explicit DBUseCase(std::unique_ptr<UnitOfWorkFactory>&& factory)
        : factory_(std::move(factory)) {
    }

    void SaveRecord(PlayerRecord&& record);
    json::value GetRecords(int start, int max_items);

private:
    json::value RecordsToJson(std::vector<PlayerRecord> records) const;

private:
    std::unique_ptr<UnitOfWorkFactory> factory_;
    static constexpr int default_max_items_output = 100;
};

class ApplicationListener {
public:
    virtual void OnTick(std::chrono::milliseconds time_delta) = 0;
};

class PlayerRetirement : public RetirementObserver {
public:
    PlayerRetirement(Players& players, PlayerTokens& player_tokens,
                     DBUseCase& db);

    void Retire(std::shared_ptr<Dog>&& dog,
                std::shared_ptr<GameSession>&& session) override;

private:
    Players& players_;
    PlayerTokens& player_tokens_;
    DBUseCase& db_;
};

class Application {
public:
    Application(Game& game, DogSpawnMode dog_spawn_mode, TickerMode ticker_mode,
                std::unique_ptr<UnitOfWorkFactory>&& factory=nullptr);

    json::value GetMaps() const;
    json::value GetMap(std::string&& map_id) const;
    json::value JoinGame(std::string_view join_info_json);
    json::value GetPlayers(std::string_view auth_json) const;
    json::value GetState(std::string_view auth_json) const;
    void DoAction(std::string_view auth_value, std::string_view action) const;
    void TickByRequest(std::string_view time_delta_in_json);
    void TickByTimer(std::chrono::milliseconds time_delta);
    void SetListener(std::shared_ptr<ApplicationListener>&& listener);
    Players& GetPlayers();
    const Players& GetPlayers() const;
    PlayerTokens& GetPlayerTokens();
    const PlayerTokens& GetPlayerTokens() const;
    json::value GetRecords(int start, int max_items);

private:
    Game& game_;
    DogSpawnMode dog_spawn_mode_;
    TickerMode ticker_mode_;

    MapsUseCase maps_;
    JoinGameUseCase join_game_;
    ListPlayersUseCase list_players_;
    GameStateUseCase game_state_;
    ActionUseCase action_;
    TickUseCase tick_;
    DBUseCase db_;

    Players players_;
    PlayerTokens player_tokens_;
    PlayerRetirement player_retirement_;

    std::shared_ptr<ApplicationListener> listener_ = nullptr;
};

}  // namespace app