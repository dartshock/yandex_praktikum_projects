#include "postgres.h"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <pqxx/result>
#include <pqxx/row>
#include <pqxx/zview.hxx>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void RecordsRepositoryImpl::AddRecord(const app::PlayerRecord& record) {
    auto id = boost::uuids::to_string(boost::uuids::random_generator()());
    auto transaction_text = R"(
        INSERT INTO retired_players (id, name, score, play_time_ms) 
        VALUES ($1, $2, $3, $4);
        )"_zv;
    transaction_.exec_params(transaction_text, id, record.name, record.score,
                             record.play_time_ms.count());
}
std::vector<app::PlayerRecord> RecordsRepositoryImpl::GetRecords(
    int start, int max_items) {
    auto db_result = transaction_.exec_params(
        "SELECT name, score, play_time_ms "
        "FROM retired_players "
        "ORDER BY score DESC, play_time_ms, name "
        "LIMIT $1 OFFSET $2",
        max_items, start);

    std::vector<app::PlayerRecord> result;
    for (const auto& row : db_result) {
        auto ms = std::chrono::milliseconds(row[2].as<int>());
        result.push_back(app::PlayerRecord{row[0].as<std::string>(),
                                           row[1].as<std::uint64_t>(),
                                           std::move(ms)});
    }
    return result;
}
UnitOfWorkFactoryImpl::UnitOfWorkFactoryImpl(
    conn_pool::ConnectionPool& conn_pool)
    : conn_pool_(conn_pool) {
    auto connection = conn_pool_.GetConnection();
    pqxx::work transaction{*connection};
    transaction.exec(R"(
        CREATE TABLE IF NOT EXISTS retired_players (
        id UUID PRIMARY KEY,
        name VARCHAR(100) NOT NULL,
        score INT NOT NULL,
        play_time_ms INT NOT NULL);
        )"_zv);
    transaction.exec(R"(
        CREATE INDEX IF NOT EXISTS score_play_time_name_idx 
        ON retired_players 
        (score DESC, play_time_ms, name);
        )"_zv);

    transaction.commit();
}
}  // namespace postgres