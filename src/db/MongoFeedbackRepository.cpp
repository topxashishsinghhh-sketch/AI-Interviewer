// OOP: Repository Pattern (MongoDB Implementation)
#include "db/MongoFeedbackRepository.h"
#include "db/Database.h"
#include "db/BSONMappers.h"
#include "db/DatabaseException.h"
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;

namespace db {

MongoFeedbackRepository::MongoFeedbackRepository(const std::string& dbName)
    : dbName_(dbName) {}

std::vector<core::FeedbackReport> MongoFeedbackRepository::findByUser(const core::UserId& userId) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["feedback"];
        
        auto filter = document{} << "userId" << userId.get() << finalize;
        auto options = mongocxx::options::find{}.sort(document{} << "timestamp" << -1 << finalize);
        
        std::vector<core::FeedbackReport> reports;
        for (auto&& doc : coll.find(filter.view(), options)) {
            reports.push_back(FeedbackMapper::fromBSON(doc));
        }
        return reports;
    } catch (const std::exception& e) {
        throw QueryException(std::string("findByUser failed: ") + e.what());
    }
}

std::vector<core::FeedbackReport> MongoFeedbackRepository::findByUserAndDateRange(
    const core::UserId& userId,
    std::chrono::system_clock::time_point from,
    std::chrono::system_clock::time_point to) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["feedback"];
        
        using bsoncxx::types::b_date;
        auto fromDate = b_date{std::chrono::duration_cast<std::chrono::milliseconds>(from.time_since_epoch())};
        auto toDate = b_date{std::chrono::duration_cast<std::chrono::milliseconds>(to.time_since_epoch())};
        
        auto filter = document{} 
            << "userId" << userId.get()
            << "timestamp" << open_document << "$gte" << fromDate << "$lte" << toDate << close_document
            << finalize;
        
        std::vector<core::FeedbackReport> reports;
        for (auto&& doc : coll.find(filter.view())) {
            reports.push_back(FeedbackMapper::fromBSON(doc));
        }
        return reports;
    } catch (const std::exception& e) {
        throw QueryException(std::string("findByUserAndDateRange failed: ") + e.what());
    }
}

void MongoFeedbackRepository::save(const core::FeedbackReport& report, 
                                   const core::UserId& userId, 
                                   const core::SessionId& sessionId) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["feedback"];
        
        auto doc = FeedbackMapper::toBSON(report, userId, sessionId);
        coll.insert_one(doc.view());
    } catch (const std::exception& e) {
        throw QueryException(std::string("save failed: ") + e.what());
    }
}

} // namespace db