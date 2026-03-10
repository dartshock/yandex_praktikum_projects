#pragma once

// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/strand.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <filesystem>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "app.h"
#include "boost_logger.h"
#include "http_server.h"
#include "model.h"
#include "utils.h"

namespace http_handler {

namespace beast = boost::beast;
namespace net = boost::asio;
namespace sys = boost::system;
namespace http = beast::http;
namespace json = boost::json;
namespace logging = boost::log;
namespace posix_time = boost::posix_time;
namespace fs = std::filesystem;
using namespace std::literals;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;
// Ответ, тело которого представлено в виде файла
using FileResponse = http::response<http::file_body>;

using Response = std::variant<StringResponse, FileResponse>;

json::value MakeErrorInJsonFormat(std::string_view code,
                                  std::string_view message);

struct GameRecordsOutputParams {
    int start = 0;
    int max_items = 100;
};

class ApiEndpoints {
public:
    bool IsGameEndpoint(std::string_view path) const;
    bool IsMapsEndpoint(std::string_view path) const;
    bool IsGameJoin(std::string_view path) const;
    bool IsGamePlayers(std::string_view path) const;
    bool IsGameState(std::string_view path) const;
    bool IsPlayerAction(std::string_view path) const;
    bool IsGameTick(std::string_view path) const;
    bool IsRecords(std::string_view path) const;
    bool IsMapsList(std::string_view path) const;
    bool IsSingleMapRequest(std::string_view path) const;

    std::string ExtractMapId(std::string_view path) const;
    GameRecordsOutputParams ExtractRecordsOutputParams(
        std::string_view path) const;
    std::string_view GetGamePrefix() const;
    std::string_view GetMapsPrefix() const;

private:
    const std::string game_prefix_{"/api/v1/game"};
    const std::string maps_prefix_{"/api/v1/maps"};

    const std::string game_join_{"/api/v1/game/join"};
    const std::string game_players_{"/api/v1/game/players"};
    const std::string game_state_{"/api/v1/game/state"};
    const std::string player_action_{"/api/v1/game/player/action"};
    const std::string game_tick_{"/api/v1/game/tick"};
    const std::string records_{"/api/v1/game/records"};
    const std::string maps_list_{"/api/v1/maps"};
};

//========== APIRequestHandler ==========
class APIRequestHandler {
public:
    APIRequestHandler(const APIRequestHandler&) = delete;
    APIRequestHandler& operator=(const APIRequestHandler&) = delete;
    explicit APIRequestHandler(model::Game& game, app::Application& app)
        : game_{game}
        , app_(app) {
    }

    StringResponse operator()(StringRequest&& req, std::string&& path);

private:
    StringResponse HandleMapsRequest(StringRequest&& req,
                                     std::string&& path) const;
    StringResponse HandleGameRequest(StringRequest&& req, std::string&& path);
    StringResponse HandleApplicationException(
        StringRequest&& req, const app::ApplicationException& exc) const;

    StringResponse BadAPIResponse(StringRequest&& req) const;
    StringResponse JoinResponse(StringRequest&& req);
    StringResponse PlayersListResponse(StringRequest&& req) const;
    StringResponse GameStateResponse(StringRequest&& req) const;
    StringResponse ActionResponse(StringRequest&& req) const;
    StringResponse TickResponse(StringRequest&& req);
    StringResponse RecordsResponse(StringRequest&& req, GameRecordsOutputParams params);

private:
    model::Game& game_;
    app::Application& app_;
    const ApiEndpoints api_endpoints_;
};

//========== FileRequestHandler ==========
class FileRequestHandler {
public:
    FileRequestHandler(const FileRequestHandler&) = delete;
    FileRequestHandler& operator=(const FileRequestHandler&) = delete;
    explicit FileRequestHandler(fs::path path)
        : root_path_{std::move(path)} {
    }

    Response operator()(StringRequest&& req, std::string path) const;

private:
    StringResponse FileOutsideRootDirResponse(StringRequest&& req) const;

    StringResponse FileNotFoundResponse(StringRequest&& req) const;

    FileResponse BuildFileResponse(StringRequest&& req, fs::path path) const;

private:
    std::filesystem::path root_path_;
};

//========== RequestHandler ==========
class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
public:
    using Strand = net::strand<net::io_context::executor_type>;

    explicit RequestHandler(model::Game& game, app::Application& app,
                            fs::path path, Strand api_strand)
        : game_{game}
        , api_strand_{api_strand}
        , api_handler_{game, app}
        , file_handler_{std::move(path)} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send, typename Log>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req,
                    Send&& send, Log&& log) {
        auto path = utils::DecodePath(std::string(req.target()));
        auto version = req.version();
        auto keep_alive = req.keep_alive();

        if (path.starts_with("/api/"s)) {
            auto handle = [self = shared_from_this(), req = std::move(req),
                           send = std::forward<Send>(send),
                           log = std::forward<Log>(log), path = std::move(path),
                           version = version,
                           keep_alive = keep_alive]() mutable {
                try {
                    auto response =
                        self->api_handler_(std::move(req), std::move(path));
                    log(response);
                    send(response);
                } catch (const std::exception& ex) {
                    auto response = RequestHandler::MakeStringResponse(
                        http::status::internal_server_error,
                        json::serialize(MakeErrorInJsonFormat("serverError"s,
                                                              "Server error"s + ex.what())),
                        version, keep_alive);
                    log(response);
                    send(response);
                }
            };
            net::dispatch(api_strand_, handle);
        } else {
            auto response = file_handler_(std::move(req), std::move(path));
            std::visit(
                [&](auto&& result) {
                    log(std::forward<decltype(result)>(result));
                    send(std::forward<decltype(result)>(result));
                },
                response);
        }
    }

    static StringResponse MakeStringResponse(
        http::status status, std::string_view body, unsigned http_version,
        bool keep_alive, std::string_view content_type = "application/json"sv);

    static FileResponse MakeFileResponse(
        http::status status, std::string path, unsigned http_version,
        bool keep_alive,
        std::string_view content_type = "application/octet-stream"sv);

    template <typename... Args>
    static StringResponse MakeResponseForAllowedMethods(StringRequest&& req,
                                                        Args&&... args) {
        std::vector<std::string> methods;
        std::ostringstream oss;
        bool first = true;

        auto add_method = [&oss, &first](const http::verb& method) {
            if (!first) {
                oss << ", ";
            }

            oss << boost::beast::http::to_string(method);
            first = false;
        };

        (add_method(args), ...);

        auto response =
            MakeStringResponse(http::status::method_not_allowed,
                               json::serialize(MakeErrorInJsonFormat(
                                   "invalidMethod"sv, "Invalid method"sv)),
                               req.version(), req.keep_alive());
        response.set(http::field::allow, oss.str());
        return response;
    }

private:
    model::Game& game_;
    Strand api_strand_;
    APIRequestHandler api_handler_;
    FileRequestHandler file_handler_;
};

//========== LoggingRequestHandler ==========
template <class RequestHandler>
class LoggingRequestHandler {
public:
    explicit LoggingRequestHandler(std::shared_ptr<RequestHandler>&& handler)
        : decorated_{std::move(handler)} {
    }

    LoggingRequestHandler(const LoggingRequestHandler&) = delete;
    LoggingRequestHandler& operator=(const LoggingRequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req,
                    Send&& send, std::string&& ip) {
        LogRequest(req, std::move(ip));

        posix_time::ptime start_time = posix_time::microsec_clock::local_time();
        auto log_wrapper = [this, &ip, start_time](auto&& response) {
            posix_time::ptime end_time =
                posix_time::microsec_clock::local_time();
            posix_time::time_duration duration = end_time - start_time;
            this->LogResponse(response, duration);
        };

        (*decorated_)(std::move(req), std::move(send), std::move(log_wrapper));
    }

private:
    void LogRequest(StringRequest& req, std::string&& ip) const {
        json::value data{{"ip"s, std::move(ip)},
                         {"URI"s, req.target()},
                         {"method"s, req.method_string()}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value("AdditionalData", data)
                                << "request received"s;
    }

    template <typename R>
    void LogResponse(const R& res,
                     const posix_time::time_duration duration) const {
        const std::string_view content_type =
            res[http::field::content_type].empty()
                ? "null"sv
                : res[http::field::content_type];

        json::value data{{"response_time"s, duration.total_milliseconds()},
                         {"code"s, res.result_int()},
                         {"content_type"s, content_type}};
        BOOST_LOG_TRIVIAL(info)
            << logging::add_value("AdditionalData", data) << "response sent"s;
    }

private:
    std::shared_ptr<RequestHandler> decorated_;
};

}  // namespace http_handler
