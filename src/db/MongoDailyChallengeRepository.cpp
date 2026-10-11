// OOP: Repository Pattern (MongoDB Implementation)
#include "db/MongoDailyChallengeRepository.h"
#include "db/Database.h"
#include "db/DatabaseException.h"
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <mongocxx/exception/bulk_write_exception.hpp>

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;

namespace db {

namespace {

// bsoncxx exposes strings as std::string_view; document::view's find() and the
// element accessors are non-const in this driver version, so the document is
// scanned with a range-for loop and each element is copied out.
bool hasKey(const bsoncxx::document::element& element, const char* key) {
    const std::string name(element.key().data(), element.key().size());
    return name == key;
}

std::string asString(const bsoncxx::document::view& view, const char* key) {
    for (bsoncxx::document::element element : view) {
        if (!hasKey(element, key)) continue;
        if (element.type() != bsoncxx::type::k_string) return std::string();
        const bsoncxx::stdx::string_view text = element.get_string().value;
        return std::string(text.data(), text.size());
    }
    return std::string();
}

std::chrono::system_clock::time_point asDate(const bsoncxx::document::view& view, const char* key) {
    const std::chrono::system_clock::time_point empty;
    for (bsoncxx::document::element element : view) {
        if (!hasKey(element, key)) continue;
        if (element.type() != bsoncxx::type::k_date) return empty;
        return std::chrono::system_clock::time_point(element.get_date().value);
    }
    return empty;
}

int asInt32(const bsoncxx::document::view& view, const char* key, int fallback) {
    for (bsoncxx::document::element element : view) {
        if (!hasKey(element, key)) continue;
        if (element.type() == bsoncxx::type::k_int32) return element.get_int32().value;
        if (element.type() == bsoncxx::type::k_int64) {
            return static_cast<int>(element.get_int64().value);
        }
        return fallback;
    }
    return fallback;
}

bool has(const bsoncxx::document::view& view, const char* key) {
    for (bsoncxx::document::element element : view) {
        if (hasKey(element, key)) return true;
    }
    return false;
}

core::DailyChallenge toChallenge(const bsoncxx::document::view& view) {
    core::DailyChallenge challenge;
    challenge.courseId = asString(view, "courseId");
    challenge.date = asDate(view, "date");
    challenge.questionId = core::QuestionId(asString(view, "questionId"));
    challenge.bonusPoints = has(view, "bonusPoints") ? asInt32(view, "bonusPoints", 10) : 10;
    return challenge;
}

} // namespace

MongoDailyChallengeRepository::MongoDailyChallengeRepository(const std::string& dbName)
    : dbName_(dbName) {}

core::Optional<core::DailyChallenge> MongoDailyChallengeRepository::findByCourseAndDate(
    const std::string& courseId, std::chrono::system_clock::time_point date) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["daily_challenges"];
        
        using bsoncxx::types::b_date;
        auto startOfDay = date - std::chrono::hours(date.time_since_epoch().count() % 86400000);
        auto endOfDay = startOfDay + std::chrono::hours(24);
        
        auto startDate = b_date{std::chrono::duration_cast<std::chrono::milliseconds>(startOfDay.time_since_epoch())};
        auto endDate = b_date{std::chrono::duration_cast<std::chrono::milliseconds>(endOfDay.time_since_epoch())};
        
        auto filter = document{} 
            << "courseId" << courseId
            << "date" << open_document << "$gte" << startDate << "$lt" << endDate << close_document
            << finalize;
        
        auto result = coll.find_one(filter.view());
        
        if (result) {
            return core::Optional<core::DailyChallenge>(toChallenge(result->view()));
        }
        return core::Optional<core::DailyChallenge>();
    } catch (const std::exception& e) {
        throw QueryException(std::string("findByCourseAndDate failed: ") + e.what());
    }
}

void MongoDailyChallengeRepository::save(const core::DailyChallenge& challenge) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["daily_challenges"];
        
        using bsoncxx::types::b_date;
        auto date = b_date{std::chrono::duration_cast<std::chrono::milliseconds>(challenge.date.time_since_epoch())};
        
        auto doc = document{}
            << "courseId" << challenge.courseId
            << "date" << date
            << "questionId" << challenge.questionId.get()
            << "bonusPoints" << challenge.bonusPoints
            << finalize;
        
        // Upsert (unique index on courseId + date)
        auto filter = document{} 
            << "courseId" << challenge.courseId
            << "date" << date
            << finalize;
        auto options = mongocxx::options::replace{}.upsert(true);
        coll.replace_one(filter.view(), doc.view(), options);
    } catch (const mongocxx::bulk_write_exception& e) {
        throw DuplicateKeyException("Daily challenge already exists for this course and date");
    } catch (const std::exception& e) {
        throw QueryException(std::string("save failed: ") + e.what());
    }
}

std::vector<core::DailyChallenge> MongoDailyChallengeRepository::findByDateRange(
    std::chrono::system_clock::time_point from,
    std::chrono::system_clock::time_point to) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["daily_challenges"];
        
        using bsoncxx::types::b_date;
        auto fromDate = b_date{std::chrono::duration_cast<std::chrono::milliseconds>(from.time_since_epoch())};
        auto toDate = b_date{std::chrono::duration_cast<std::chrono::milliseconds>(to.time_since_epoch())};
        
        auto filter = document{} 
            << "date" << open_document << "$gte" << fromDate << "$lte" << toDate << close_document
            << finalize;
        
        std::vector<core::DailyChallenge> challenges;
        for (auto&& doc : coll.find(filter.view())) {
            // The cursor yields bsoncxx::document::view directly.
            challenges.push_back(toChallenge(doc));
        }
        return challenges;
    } catch (const std::exception& e) {
        throw QueryException(std::string("findByDateRange failed: ") + e.what());
    }
}

} // namespace db