#include "request_handler.h"

#include <boost/beast/core/file_stdio.hpp>
#include <regex>
#include <string>

namespace http_handler {

bool ApiEndpoints::IsGameEndpoint(std::string_view path) const {
    return path.starts_with(game_prefix_);
}

bool ApiEndpoints::IsMapsEndpoint(std::string_view path) const {
    return path.starts_with(maps_prefix_);
}

bool ApiEndpoints::IsGameJoin(std::string_view path) const {
    return path == game_join_;
}

bool ApiEndpoints::IsGamePlayers(std::string_view path) const {
    return path == game_players_;
}

bool ApiEndpoints::IsGameState(std::string_view path) const {
    return path == game_state_;
}

bool ApiEndpoints::IsPlayerAction(std::string_view path) const {
    return path == player_action_;
}

bool ApiEndpoints::IsGameTick(std::string_view path) const {
    return path == game_tick_;
}

bool ApiEndpoints::IsRecords(std::string_view path) const {
    return path.starts_with(path);
}

bool ApiEndpoints::IsMapsList(std::string_view path) const {
    return path == maps_list_;
}

bool ApiEndpoints::IsSingleMapRequest(std::string_view path) const {
    if (!path.starts_with(maps_prefix_)) {
        return false;
    }
    return path.size() > maps_prefix_.size();
}

std::string ApiEndpoints::ExtractMapId(std::string_view path) const {
    if (path.starts_with(maps_prefix_)) {
        return std::string(path.substr(maps_prefix_.length() + 1));
    }
    return "";
}

GameRecordsOutputParams ApiEndpoints::ExtractRecordsOutputParams(
    std::string_view path) const {
    GameRecordsOutputParams params;

    std::regex start_regex(R"([?&]start=(\d+)(?=&|$))");
    std::regex maxItems_regex(R"([?&]maxItems=(\d+)(?=&|$))");

    std::smatch match;
    std::string path_str = std::string(path);

    if (std::regex_search(path_str, match, start_regex) && match.size() > 1) {
        params.start = std::stoi(match[1].str());
    }

    if (std::regex_search(path_str, match, maxItems_regex) &&
        match.size() > 1) {
        params.max_items = std::stoi(match[1].str());
    }
    return params;
}

std::string_view ApiEndpoints::GetGamePrefix() const {
    return game_prefix_;
}

std::string_view ApiEndpoints::GetMapsPrefix() const {
    return maps_prefix_;
}

json::value MakeErrorInJsonFormat(std::string_view code,
                                  std::string_view message) {
    return json::value{{"code"s, code}, {"message"s, message}};
}

//========== APIRequestHandler ==========
StringResponse APIRequestHandler::operator()(StringRequest&& req,
                                             std::string&& path) {
    if (api_endpoints_.IsMapsEndpoint(path)) {
        return HandleMapsRequest(std::move(req), std::move(path));
    }

    if (api_endpoints_.IsGameEndpoint(path)) {
        return HandleGameRequest(std::move(req), std::move(path));
    }

    return BadAPIResponse(std::move(req));
}

StringResponse APIRequestHandler::HandleMapsRequest(StringRequest&& req,
                                                    std::string&& path) const {
    if (!(req.method() == http::verb::get ||
          req.method() == http::verb::head)) {
        return RequestHandler::MakeResponseForAllowedMethods(
            std::move(req), http::verb::get, http::verb::head);
    }
    if (api_endpoints_.IsMapsList(path)) {
        return RequestHandler::MakeStringResponse(
            http::status::ok, json::serialize(app_.GetMaps()), req.version(),
            req.keep_alive());
    }

    std::string map_id = api_endpoints_.ExtractMapId(path);
    try {
        return RequestHandler::MakeStringResponse(
            http::status::ok, json::serialize(app_.GetMap(std::move(map_id))),
            req.version(), req.keep_alive());
    } catch (const app::ApplicationException& exc) {
        return HandleApplicationException(std::move(req), exc);
    }

    return BadAPIResponse(std::move(req));
}

StringResponse APIRequestHandler::HandleGameRequest(StringRequest&& req,
                                                    std::string&& path) {
    if (api_endpoints_.IsGameJoin(path)) {
        return JoinResponse(std::move(req));
    }

    if (api_endpoints_.IsGamePlayers(path)) {
        return PlayersListResponse(std::move(req));
    }

    if (api_endpoints_.IsGameState(path)) {
        return GameStateResponse(std::move(req));
    }

    if (api_endpoints_.IsPlayerAction(path)) {
        return ActionResponse(std::move(req));
    }

    if (api_endpoints_.IsGameTick(path)) {
        return TickResponse(std::move(req));
    }

    if (api_endpoints_.IsRecords(path)) {
        return RecordsResponse(std::move(req), api_endpoints_.ExtractRecordsOutputParams(path));
    }

    return BadAPIResponse(std::move(req));
}

StringResponse APIRequestHandler::HandleApplicationException(
    StringRequest&& req, const app::ApplicationException& exc) const {
    using Error = app::ApplicationException::Error;
    switch (exc.GetError()) {
        case Error::Parse:
            return RequestHandler::MakeStringResponse(
                http::status::bad_request,
                json::serialize(
                    MakeErrorInJsonFormat("invalidArgument"s, exc.ToString())),
                req.version(), req.keep_alive());
        case Error::InvalidToken:
            return RequestHandler::MakeStringResponse(
                http::status::unauthorized,
                json::serialize(
                    MakeErrorInJsonFormat("invalidToken"s, exc.ToString())),
                req.version(), req.keep_alive());
        case Error::UnknownToken:
            return RequestHandler::MakeStringResponse(
                http::status::unauthorized,
                json::serialize(
                    MakeErrorInJsonFormat("unknownToken"s, exc.ToString())),
                req.version(), req.keep_alive());
        case Error::UnknownMap:
            return RequestHandler::MakeStringResponse(
                http::status::not_found,
                json::serialize(
                    MakeErrorInJsonFormat("mapNotFound"s, exc.ToString())),
                req.version(), req.keep_alive());
        case Error::Tick:
            return RequestHandler::MakeStringResponse(
                http::status::bad_request,
                json::serialize(
                    MakeErrorInJsonFormat("badRequest"s, exc.ToString())),
                req.version(), req.keep_alive());
        case Error::DataBase:
            return RequestHandler::MakeStringResponse(
                http::status::bad_request,
                json::serialize(
                    MakeErrorInJsonFormat("badRequest"s, exc.ToString())),
                req.version(), req.keep_alive());
        default:
            return RequestHandler::MakeStringResponse(
                http::status::internal_server_error,
                json::serialize(
                    MakeErrorInJsonFormat("serverError"s, "unexpected error"s)),
                req.version(), req.keep_alive());
    }
}

StringResponse APIRequestHandler::BadAPIResponse(StringRequest&& req) const {
    return RequestHandler::MakeStringResponse(
        http::status::bad_request,
        json::serialize(MakeErrorInJsonFormat("badRequest"sv, "Bad request"sv)),
        req.version(), req.keep_alive());
}

StringResponse APIRequestHandler::JoinResponse(StringRequest&& req) {
    if (req.method() != http::verb::post) {
        return RequestHandler::MakeResponseForAllowedMethods(std::move(req),
                                                             http::verb::post);
    }

    try {
        return RequestHandler::MakeStringResponse(
            http::status::ok, json::serialize(app_.JoinGame(req.body())),
            req.version(), req.keep_alive());

    } catch (const app::ApplicationException& exc) {
        return HandleApplicationException(std::move(req), exc);
    }
}

StringResponse APIRequestHandler::PlayersListResponse(
    StringRequest&& req) const {
    if (!(req.method() == http::verb::get ||
          req.method() == http::verb::head)) {
        return RequestHandler::MakeResponseForAllowedMethods(
            std::move(req), http::verb::get, http::verb::head);
    }

    auto auth_iter = req.find("Authorization"s);
    if (auth_iter == req.end()) {
        return RequestHandler::MakeStringResponse(
            http::status::unauthorized,
            json::serialize(MakeErrorInJsonFormat(
                "invalidToken"s, "Authorization header is missing"s)),
            req.version(), req.keep_alive());
    }

    try {
        return RequestHandler::MakeStringResponse(
            http::status::ok,
            json::serialize(app_.GetPlayers(auth_iter->value())), req.version(),
            req.keep_alive());
    } catch (const app::ApplicationException& exc) {
        return HandleApplicationException(std::move(req), exc);
    }
}

StringResponse APIRequestHandler::GameStateResponse(StringRequest&& req) const {
    if (!(req.method() == http::verb::get ||
          req.method() == http::verb::head)) {
        return RequestHandler::MakeResponseForAllowedMethods(
            std::move(req), http::verb::get, http::verb::head);
    }

    auto auth_iter = req.find("Authorization");
    if (auth_iter == req.end()) {
        return RequestHandler::MakeStringResponse(
            http::status::unauthorized,
            json::serialize(MakeErrorInJsonFormat(
                "invalidToken"s, "Authorization header is missing"s)),
            req.version(), req.keep_alive());
    }

    try {
        return RequestHandler::MakeStringResponse(
            http::status::ok,
            json::serialize(app_.GetState(auth_iter->value())), req.version(),
            req.keep_alive());
    } catch (const app::ApplicationException& exc) {
        return HandleApplicationException(std::move(req), exc);
    }
}

StringResponse APIRequestHandler::ActionResponse(StringRequest&& req) const {
    if (req.method() != http::verb::post) {
        return RequestHandler::MakeResponseForAllowedMethods(std::move(req),
                                                             http::verb::post);
    }

    auto content_iter = req.find(http::field::content_type);
    if (content_iter == req.end()) {
        return RequestHandler::MakeStringResponse(
            http::status::unauthorized,
            json::serialize(MakeErrorInJsonFormat("invalidArgument"s,
                                                  "Invalid content type"s)),
            req.version(), req.keep_alive());
    }

    auto auth_iter = req.find("Authorization"s);
    if (auth_iter == req.end()) {
        return RequestHandler::MakeStringResponse(
            http::status::unauthorized,
            json::serialize(MakeErrorInJsonFormat(
                "invalidToken"s, "Authorization header is missing"s)),
            req.version(), req.keep_alive());
    }

    try {
        app_.DoAction(auth_iter->value(), req.body());
        return RequestHandler::MakeStringResponse(
            http::status::ok, "{}"s, req.version(), req.keep_alive());
    } catch (const app::ApplicationException& exc) {
        return HandleApplicationException(std::move(req), exc);
    }
}

StringResponse APIRequestHandler::TickResponse(StringRequest&& req) {
    if (req.method() != http::verb::post) {
        return RequestHandler::MakeResponseForAllowedMethods(std::move(req),
                                                             http::verb::post);
    }

    try {
        app_.TickByRequest(req.body());
        return RequestHandler::MakeStringResponse(
            http::status::ok, "{}"s, req.version(), req.keep_alive());
    } catch (const app::ApplicationException& exc) {
        return HandleApplicationException(std::move(req), exc);
    }
}

StringResponse APIRequestHandler::RecordsResponse(StringRequest&& req, GameRecordsOutputParams params) {
    if (!(req.method() == http::verb::get ||
          req.method() == http::verb::head)) {
        return RequestHandler::MakeResponseForAllowedMethods(
            std::move(req), http::verb::get, http::verb::head);
    }

    try {
        return RequestHandler::MakeStringResponse(
            http::status::ok,
            json::serialize(app_.GetRecords(params.start, params.max_items)),
            req.version(), req.keep_alive());
    } catch (const app::ApplicationException& exc) {
        return HandleApplicationException(std::move(req), exc);
    }
}

//========== FileRequestHandler ==========
Response FileRequestHandler::operator()(StringRequest&& req,
                                        std::string path) const {
    if (path == "/"s || path.empty()) {
        return BuildFileResponse(std::move(req), root_path_ / "index.html"s);
    }
    const fs::path full_path = root_path_ / path.substr(1);
    if (!utils::IsSubPath(full_path, root_path_)) {
        return FileOutsideRootDirResponse(std::move(req));
    }

    if (!fs::exists(full_path)) {
        return FileNotFoundResponse(std::move(req));
    }

    return BuildFileResponse(std::move(req), std::move(full_path));
}

StringResponse FileRequestHandler::FileOutsideRootDirResponse(
    StringRequest&& req) const {
    return RequestHandler::MakeStringResponse(
        http::status::bad_request, "File outside root directory"sv,
        req.version(), req.keep_alive(), "text/plain"sv);
}

StringResponse FileRequestHandler::FileNotFoundResponse(
    StringRequest&& req) const {
    return RequestHandler::MakeStringResponse(http::status::not_found,
                                              "File not found"sv, req.version(),
                                              req.keep_alive(), "text/plain"sv);
}

FileResponse FileRequestHandler::BuildFileResponse(StringRequest&& req,
                                                   fs::path path) const {
    return RequestHandler::MakeFileResponse(
        http::status::ok, path.string(), req.version(), req.keep_alive(),
        utils::GetMIMEType(path.extension()));
}

//========== RequestHandler ==========
StringResponse RequestHandler::MakeStringResponse(
    http::status status, std::string_view body, unsigned http_version,
    bool keep_alive, std::string_view content_type) {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    response.set(http::field::cache_control, "no-cache"sv);

    return response;
}

FileResponse RequestHandler::MakeFileResponse(http::status status,
                                              std::string path,
                                              unsigned http_version,
                                              bool keep_alive,
                                              std::string_view content_type) {
    FileResponse response(status, http_version);
    response.set(http::field::content_type, content_type);

    http::file_body::value_type file;
    if (sys::error_code ec;
        file.open(path.c_str(), beast::file_mode::read, ec), ec) {
        throw std::runtime_error("Failed to open file: "s + path);
    }
    response.body() = std::move(file);
    response.prepare_payload();
    response.keep_alive(keep_alive);

    return response;
}

}  // namespace http_handler
