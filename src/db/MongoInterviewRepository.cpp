// OOP: Repository Pattern (MongoDB Implementation)
#include "db/MongoInterviewRepository.h"
#include "db/Database.h"
#include "db/BSONMappers.h"
#include "db/DatabaseException.h"
#include <bsoncxx/builder/stream/document.hpp>

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;

namespace db {

MongoInterviewRepository::MongoInterviewRepository(const std::string& dbName)
    : dbName_(dbName) {}

std::shared_ptr<core::InterviewSession> MongoInterviewRepository::findById(const core::SessionId& id) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["interviews"];
        
        auto filter = document{} << "_id" << id.get() << finalize;
        auto result = coll.find_one(filter.view());
        
        if (result) {
            return InterviewSessionMapper::fromBSON(result->view());
        }
        return nullptr;
    } catch (const std::exception& e) {
        throw QueryException(std::string("findById failed: ") + e.what());
    }
}

std::vector<std::shared_ptr<core::InterviewSession>> MongoInterviewRepository::findByUser(const core::UserId& userId) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["interviews"];
        
        auto filter = document{} << "userId" << userId.get() << finalize;
        auto options = mongocxx::options::find{}.sort(document{} << "startTime" << -1 << finalize);
        
        std::vector<std::shared_ptr<core::InterviewSession>> sessions;
        for (auto&& doc : coll.find(filter.view(), options)) {
            auto session = InterviewSessionMapper::fromBSON(doc);
            if (session) sessions.push_back(session);
        }
        return sessions;
    } catch (const std::exception& e) {
        throw QueryException(std::string("findByUser failed: ") + e.what());
    }
}

void MongoInterviewRepository::save(const std::shared_ptr<core::InterviewSession>& session) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["interviews"];
        
        auto doc = InterviewSessionMapper::toBSON(*session);
        coll.insert_one(doc.view());
    } catch (const std::exception& e) {
        throw QueryException(std::string("save failed: ") + e.what());
    }
}

void MongoInterviewRepository::update(const std::shared_ptr<core::InterviewSession>& session) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["interviews"];
        
        auto doc = InterviewSessionMapper::toBSON(*session);
        auto filter = document{} << "_id" << session->id().get() << finalize;
        coll.replace_one(filter.view(), doc.view());
    } catch (const std::exception& e) {
        throw QueryException(std::string("update failed: ") + e.what());
    }
}

} // namespace db