#pragma once

#include <pqxx/connection>
#include <pqxx/transaction>

#include "../app.h"
#include "../connection_pool.h"

namespace postgres {
    
using namespace app;

class RecordsRepositoryImpl : public app::RecordsRepository {
public:
    explicit RecordsRepositoryImpl(pqxx::work& transaction)
        : transaction_{transaction} {
    }

    void AddRecord(const app::PlayerRecord& record) override;
    std::vector<app::PlayerRecord> GetRecords(int start, int max_items) override;

private:
    pqxx::work& transaction_;
};

class UnitOfWorkImpl : public app::UnitOfWork {
public:
    explicit UnitOfWorkImpl(
        conn_pool::ConnectionPool::ConnectionWrapper&& connection)
        : connection_(std::move(connection))
        , transaction_(*connection_)
        , records_(transaction_) {
    }

    app::RecordsRepository& Records() override {
        return records_;
    }
    void Commit() override {
        transaction_.commit();
    }
    void Rollback() override {
        transaction_.abort();
    }

private:
    conn_pool::ConnectionPool::ConnectionWrapper connection_;
    pqxx::work transaction_;

    RecordsRepositoryImpl records_;
};

class UnitOfWorkFactoryImpl : public app::UnitOfWorkFactory {
public:
    explicit UnitOfWorkFactoryImpl(conn_pool::ConnectionPool& conn_pool);

    std::unique_ptr<app::UnitOfWork> GetUnitOfWork() override {
        return std::make_unique<UnitOfWorkImpl>(
            std::move(conn_pool_.GetConnection()));
    }

private:
    conn_pool::ConnectionPool& conn_pool_;
};
}  // namespace postgres
