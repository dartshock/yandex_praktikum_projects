#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <boost/json.hpp>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <thread>

#include "app.h"
#include "boost_logger.h"
#include "connection_pool.h"
#include "json_loader.h"
#include "model.h"
#include "model_serialization.h"
#include "postgres/postgres.h"
#include "program_options.h"
#include "request_handler.h"
#include "sdk.h"
#include "ticker.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace json = boost::json;
namespace logging = boost::log;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

constexpr const char DB_URL_ENV_NAME[]{"GAME_DB_URL"};

}  // namespace

int main(int argc, const char* argv[]) {
    try {
        if (auto args = program_options::ParseCommandLine(argc, argv)) {
            // Загружаем карту из файла и строим модель игры
            model::Game game = json_loader::LoadGame(args->config_path);

            // Проверяем режим спавна собак
            app::DogSpawnMode dog_spawn_mode =
                args->random_spawn ? app::DogSpawnMode::Random
                                   : app::DogSpawnMode::AtFirstRoadStart;

            // Проверяем ticker mode
            app::TickerMode ticker_mode = args->tick_period
                                              ? app::TickerMode::Auto
                                              : app::TickerMode::ByRequest;
            // Создаем пул соединений
            const char* db_url = std::getenv(DB_URL_ENV_NAME);
            // std::cerr << db_url<< std::endl;
            if (!db_url) {
                // std::cerr << "wrong variable" << std::endl;
                throw std::runtime_error(DB_URL_ENV_NAME +
                                         " environment variable not found"s);
            }
            const unsigned num_threads = std::thread::hardware_concurrency();
            conn_pool::ConnectionPool conn_pool{
                num_threads, [db_url] {
                    return std::make_shared<pqxx::connection>(db_url);
                }};

            // Создаем приложение
            app::Application app{
                game, dog_spawn_mode, ticker_mode,
                std::make_unique<postgres::UnitOfWorkFactoryImpl>(conn_pool)};

            // Создаем слушателя
            std::shared_ptr<serialization::SerializingListener> ser_listener =
                nullptr;
            // Проверяем state file
            if (args->state_path) {
                ser_listener =
                    std::make_shared<serialization::SerializingListener>(
                        game, app, args->state_path.value());
                ser_listener->RestoreState();
                // Проверяем save period
                if (args->save_period) {
                    ser_listener->SetSavePeriod(
                        std::chrono::milliseconds(args->save_period.value()));
                    app.SetListener(ser_listener);
                }
            }

            // Инициализируем io_context
            net::io_context ioc(num_threads);

            // Добавляем strand для выполнения запросов к API
            auto api_strand = net::make_strand(ioc);

            std::shared_ptr<ticker::Ticker> ticker;
            // Добавляем тикер
            if (args->tick_period) {
                ticker = std::make_shared<ticker::Ticker>(
                    api_strand,
                    std::chrono::milliseconds(args->tick_period.value()),
                    [&app](std::chrono::milliseconds delta) {
                        app.TickByTimer(delta);
                    });
                ticker->Start();
            }

            // Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
            net::signal_set signals(ioc, SIGINT, SIGTERM);
            signals.async_wait([&ioc](const sys::error_code& ec,
                                      [[maybe_unused]] int signal_number) {
                if (!ec) {
                    BOOST_LOG_TRIVIAL(info)
                        << logging::add_value("AdditionalData",
                                              json::value{{"code"s, 0}})
                        << "server exited"s;
                    ioc.stop();
                }
            });

            // Создаём обработчик HTTP-запросов и связываем его с моделью игры
            auto handler = std::make_shared<http_handler::RequestHandler>(
                game, app, args->static_path, api_strand);
            http_handler::LoggingRequestHandler logging_handler{
                std::move(handler)};

            // Запустить обработчик HTTP-запросов, делегируя их обработчику
            // запросов
            const auto address = net::ip::make_address("0.0.0.0");
            constexpr net::ip::port_type port = 8080;
            http_server::ServeHttp(
                ioc, {address, port},
                [&logging_handler](auto&& req, auto&& send, auto&& ip) {
                    logging_handler(std::forward<decltype(req)>(req),
                                    std::forward<decltype(send)>(send),
                                    std::forward<decltype(ip)>(ip));
                });

            // Эта надпись сообщает тестам о том, что сервер запущен и готов
            // обрабатывать запросы
            boost_logger::InitBoostLogFilter();
            BOOST_LOG_TRIVIAL(info)
                << logging::add_value(
                       "AdditionalData",
                       json::value{{"port"s, port},
                                   {"address"s, address.to_string()}})
                << "server started"s;

            // Запускаем обработку асинхронных операций
            RunWorkers(std::max(1u, num_threads), [&ioc] { ioc.run(); });

            // Cохраняем состояние
            if (ser_listener) {
                ser_listener->SaveState();
            }
        }

    } catch (const std::exception& ex) {
        BOOST_LOG_TRIVIAL(info)
            << logging::add_value("AdditionalData",
                                  json::value{{"code"s, "EXIT_FAILURE"s},
                                              {"exception"s, ex.what()}})
            << "server exited"s;
        return EXIT_FAILURE;
    }
}
