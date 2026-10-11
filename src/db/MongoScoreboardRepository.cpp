// OOP: Repository Pattern (MongoDB Implementation)
#include "db/MongoScoreboardRepository.h"
#include "db/Database.h"
#include "db/DatabaseException.h"
#include "db/BSONMappers.h"
#include <bsoncxx/builder/stream/document.hpp>

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;

namespace db {

namespace {

// bsoncxx exposes strings as std::string_view and dates as an int64 millisecond
// count. The document is scanned with a range-for loop because
// document::view::find() and the element accessors are non-const in this
// driver version; element copies are taken from the loop for the same reason.
bool isKey(const bsoncxx::document::element& element, const char* key) {
    const std::string name(element.key().data(), element.key().size());
    return name == key;
}

std::string asString(const bsoncxx::document::view& view, const char* key) {
    for (bsoncxx::document::element element : view) {
        if (!isKey(element, key)) continue;
        if (element.type() != bsoncxx::type::k_string) return std::string();
        const bsoncxx::stdx::string_view text = element.get_string().value;
        return std::string(text.data(), text.size());
    }
    return std::string();
}

std::chrono::system_clock::time_point asDate(const bsoncxx::document::view& view, const char* key) {
    const std::chrono::system_clock::time_point empty;
    for (bsoncxx::document::element element : view) {
        if (!isKey(element, key)) continue;
        if (element.type() != bsoncxx::type::k_date) return empty;
        return std::chrono::system_clock::time_point(element.get_date().value);
    }
    return empty;
}

double asDouble(const bsoncxx::document::view& view, const char* key, double fallback = 0.0) {
    for (bsoncxx::document::element element : view) {
        if (!isKey(element, key)) continue;
        if (element.type() == bsoncxx::type::k_double) return element.get_double().value;
        if (element.type() == bsoncxx::type::k_int32) {
            return static_cast<double>(element.get_int32().value);
        }
        if (element.type() == bsoncxx::type::k_int64) {
            return static_cast<double>(element.get_int64().value);
        }
        return fallback;
    }
    return fallback;
}

int asInt32(const bsoncxx::document::view& view, const char* key, int fallback = 0) {
    for (bsoncxx::document::element element : view) {
        if (!isKey(element, key)) continue;
        if (element.type() == bsoncxx::type::k_int32) return element.get_int32().value;
        if (element.type() == bsoncxx::type::k_int64) {
            return static_cast<int>(element.get_int64().value);
        }
        if (element.type() == bsoncxx::type::k_double) {
            return static_cast<int>(element.get_double().value);
        }
        return fallback;
    }
    return fallback;
}

core::ScoreboardEntry toEntry(const bsoncxx::document::view& view) {
    core::ScoreboardEntry entry;
    entry.userId = core::UserId(asString(view, "userId"));
    entry.name = asString(view, "name");
    entry.college = asString(view, "college");
    entry.branch = asString(view, "branch");
    entry.score = asDouble(view, "score");
    entry.solved = asInt32(view, "solved");
    entry.streak = asInt32(view, "streak");
    entry.lastActivity = asDate(view, "lastActivity");
    entry.rank = asInt32(view, "rank");
    return entry;
}

} // namespace

MongoScoreboardRepository::MongoScoreboardRepository(const std::string& dbName)
    : dbName_(dbName) {}

std::string MongoScoreboardRepository::collectionName(const std::string& scope, const std::string& window, const std::string& ranking) const {
    return "scoreboards_" + scope + "_" + window + "_" + ranking;
}

void MongoScoreboardRepository::upsert(const core::ScoreboardEntry& entry, 
                                       const std::string& scope,
                                       const std::string& window,
                                       const std::string& ranking) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_][collectionName(scope, window, ranking)];
        
        auto doc = ScoreboardMapper::toBSON(
            entry.userId.get(), entry.name, entry.college, entry.branch,
            entry.score, entry.solved, entry.streak, entry.lastActivity,
            scope, window, ranking, entry.rank);
        
        auto filter = document{} << "userId" << entry.userId.get() << finalize;
        auto options = mongocxx::options::replace{}.upsert(true);
        coll.replace_one(filter.view(), doc.view(), options);
    } catch (const std::exception& e) {
        throw QueryException(std::string("upsert failed: ") + e.what());
    }
}

std::vector<core::ScoreboardEntry> MongoScoreboardRepository::findTop(int n,
                                          const std::string& scope,
                                          const std::string& window,
                                          const std::string& ranking) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_][collectionName(scope, window, ranking)];
        
        auto options = mongocxx::options::find{}
            .sort(document{} << "rank" << 1 << finalize)
            .limit(n);
        
        std::vector<core::ScoreboardEntry> entries;
        for (auto&& doc : coll.find({}, options)) {
            // The cursor yields bsoncxx::document::view directly.
            entries.push_back(toEntry(doc));
        }
        return entries;
    } catch (const std::exception& e) {
        throw QueryException(std::string("findTop failed: ") + e.what());
    }
}

core::Optional<core::ScoreboardEntry> MongoScoreboardRepository::findUser(const core::UserId& userId,
                                           const std::string& scope,
                                           const std::string& window,
                                           const std::string& ranking) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_][collectionName(scope, window, ranking)];
        
        auto filter = document{} << "userId" << userId.get() << finalize;
        auto result = coll.find_one(filter.view());
        
        if (result) {
            return core::Optional<core::ScoreboardEntry>(toEntry(result->view()));
        }
        return core::Optional<core::ScoreboardEntry>();
    } catch (const std::exception& e) {
        throw QueryException(std::string("findUser failed: ") + e.what());
    }
}

int MongoScoreboardRepository::getUserRank(const core::UserId& userId,
                                           const std::string& scope,
                                           const std::string& window,
                                           const std::string& ranking) {
    auto entry = findUser(userId, scope, window, ranking);
    return entry ? entry->rank : -1;
}

} // namespace db