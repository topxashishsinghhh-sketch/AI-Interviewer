// OOP: Repository Pattern (MongoDB Implementation)
#include "db/MongoAttemptRepository.h"
#include "db/Database.h"
#include "db/BSONMappers.h"
#include "db/DatabaseException.h"
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <mongocxx/exception/bulk_write_exception.hpp>

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;

namespace db {

MongoAttemptRepository::MongoAttemptRepository(const std::string& dbName)
    : dbName_(dbName) {}

core::Attempt* MongoAttemptRepository::findById(const core::AttemptId& id) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["attempts"];
        
        auto filter = document{} << "_id" << id.get() << finalize;
        auto result = coll.find_one(filter.view());
        
        if (result) {
            // Note: This returns a pointer to a local object - in production, use shared_ptr
            static thread_local core::Attempt lastResult;
            lastResult = AttemptMapper::fromBSON(result->view());
            return &lastResult;
        }
        return nullptr;
    } catch (const std::exception& e) {
        throw QueryException(std::string("findById failed: ") + e.what());
    }
}

std::vector<core::Attempt> MongoAttemptRepository::findByUser(const core::UserId& userId) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["attempts"];
        
        auto filter = document{} << "userId" << userId.get() << finalize;
        auto options = mongocxx::options::find{}.sort(document{} << "timestamp" << -1 << finalize);
        
        std::vector<core::Attempt> attempts;
        for (auto&& doc : coll.find(filter.view(), options)) {
            attempts.push_back(AttemptMapper::fromBSON(doc));
        }
        return attempts;
    } catch (const std::exception& e) {
        throw QueryException(std::string("findByUser failed: ") + e.what());
    }
}

std::vector<core::Attempt> MongoAttemptRepository::findByUserAndDateRange(
    const core::UserId& userId,
    std::chrono::system_clock::time_point from,
    std::chrono::system_clock::time_point to) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["attempts"];
        
        using bsoncxx::types::b_date;
        auto fromDate = b_date{std::chrono::duration_cast<std::chrono::milliseconds>(from.time_since_epoch())};
        auto toDate = b_date{std::chrono::duration_cast<std::chrono::milliseconds>(to.time_since_epoch())};
        
        auto filter = document{} 
            << "userId" << userId.get()
            << "timestamp" << open_document << "$gte" << fromDate << "$lte" << toDate << close_document
            << finalize;
        
        std::vector<core::Attempt> attempts;
        for (auto&& doc : coll.find(filter.view())) {
            attempts.push_back(AttemptMapper::fromBSON(doc));
        }
        return attempts;
    } catch (const std::exception& e) {
        throw QueryException(std::string("findByUserAndDateRange failed: ") + e.what());
    }
}

std::vector<core::Attempt> MongoAttemptRepository::findByQuestion(const core::QuestionId& questionId) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["attempts"];
        
        auto filter = document{} << "questionId" << questionId.get() << finalize;
        
        std::vector<core::Attempt> attempts;
        for (auto&& doc : coll.find(filter.view())) {
            attempts.push_back(AttemptMapper::fromBSON(doc));
        }
        return attempts;
    } catch (const std::exception& e) {
        throw QueryException(std::string("findByQuestion failed: ") + e.what());
    }
}

void MongoAttemptRepository::save(const core::Attempt& attempt) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["attempts"];
        
        auto doc = AttemptMapper::toBSON(attempt);
        
        // Insert (attempts are immutable once created)
        coll.insert_one(doc.view());
    } catch (const std::exception& e) {
        throw QueryException(std::string("save failed: ") + e.what());
    }
}

void MongoAttemptRepository::remove(const core::AttemptId& id) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["attempts"];
        
        auto filter = document{} << "_id" << id.get() << finalize;
        coll.delete_one(filter.view());
    } catch (const std::exception& e) {
        throw QueryException(std::string("remove failed: ") + e.what());
    }
}

} // namespace db