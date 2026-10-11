// OOP: Singleton Pattern (Database Connection Manager)
#include "db/Database.h"
#include "db/DatabaseException.h"
#include <bsoncxx/builder/stream/document.hpp>
#include <set>
#include <string>
#include <vector>

namespace db {

void Database::initialize(const std::string& uri, const std::string& dbName) {
    std::lock_guard<std::mutex> lock(initMutex_);
    
    if (initialized_) {
        return; // Already initialized
    }
    
    try {
        uri_ = uri;
        dbName_ = dbName;

        // Initialize MongoDB driver
        instance_ = std::make_unique<mongocxx::instance>();
        pool_ = std::make_unique<mongocxx::pool>(mongocxx::uri{uri_});

        // Test connection (uses the pool directly; see ping()).
        if (!ping()) {
            throw ConnectionException("Failed to ping database");
        }

        // The pool is usable, so the instance is now able to serve clients and
        // the schema setup below can go through acquire().
        initialized_ = true;

        // Create collections with validators
        createCollections();
    } catch (const std::exception& e) {
        // Leave the singleton in a usable-but-empty state rather than half-configured.
        initialized_ = false;
        pool_.reset();
        instance_.reset();
        throw ConnectionException(std::string("Failed to initialize database: ") + e.what());
    }
}

mongocxx::pool::entry Database::acquire() {
    if (!initialized_) {
        throw ConnectionException("Database not initialized");
    }
    return pool_->acquire();
}

bool Database::ping() {
    // Uses the pool directly rather than acquire(), because acquire() refuses
    // to hand out clients while initialized_ is still false and this runs from
    // inside initialize().
    try {
        if (!pool_) {
            return false;
        }
        auto client = pool_->acquire();
        client->database("admin").run_command(
            bsoncxx::builder::stream::document{} << "ping" << 1
                                                 << bsoncxx::builder::stream::finalize);
        return true;
    } catch (...) {
        return false;
    }
}

IndexSpec makeIndex(const char* name,
                    std::vector<const char*> fields,
                    std::vector<int> directions,
                    bool unique) {
    IndexSpec spec;
    spec.name = name;
    spec.fields = std::move(fields);
    spec.directions = std::move(directions);
    spec.unique = unique;
    return spec;
}

CollectionSpec makeCollection(const char* name, std::vector<IndexSpec> indexes) {
    CollectionSpec spec;
    spec.name = name;
    spec.indexes = std::move(indexes);
    return spec;
}

std::vector<CollectionSpec> Database::collectionSpecs() {
    // Each spec mirrors what its repository queries: unique constraints protect
    // invariants (one account per email, one challenge per course+date), and the
    // remaining indexes cover the lookups and sort orders used by the repos.
    std::vector<CollectionSpec> specs;
    specs.push_back(makeCollection("users", {makeIndex("users_email", {"email"}, {1}, true)}));
    specs.push_back(makeCollection("questions", {
        makeIndex("questions_subject_difficulty", {"subject", "difficulty"}, {1, 1}, false),
        makeIndex("questions_topic", {"topic"}, {1}, false)
    }));
    specs.push_back(makeCollection("attempts", {
        makeIndex("attempts_userId_timestamp", {"userId", "timestamp"}, {1, -1}, false),
        makeIndex("attempts_questionId", {"questionId"}, {1}, false)
    }));
    specs.push_back(makeCollection("interviews", {
        makeIndex("interviews_userId_startTime", {"userId", "startTime"}, {1, -1}, false)
    }));
    specs.push_back(makeCollection("feedback", {
        makeIndex("feedback_userId", {"userId"}, {1}, false)
    }));
    specs.push_back(makeCollection("scoreboards_global_alltime_score", {
        makeIndex("scoreboards_userId", {"userId"}, {1}, true)
    }));
    specs.push_back(makeCollection("daily_challenges", {
        makeIndex("daily_challenges_courseId_date", {"courseId", "date"}, {1, 1}, true)
    }));
    return specs;
}

void Database::createCollections() {
    auto client = acquire();
    auto db = client->database(dbName_);

    // Names of the collections that already exist, so creation is idempotent.
    std::set<std::string> existing;
    for (const bsoncxx::document::view& info : db.list_collections()) {
        existing.insert(std::string(info["name"].get_string().value));
    }

    const std::vector<CollectionSpec> specs = collectionSpecs();
    for (const CollectionSpec& spec : specs) {
        if (existing.find(spec.name) == existing.end()) {
            // Schemas live in the domain layer, so no collection-level validator
            // is installed; only the indexes the repositories rely on matter.
            try {
                db.create_collection(spec.name);
            } catch (const std::exception&) {
                // Another process may have created it concurrently.
            }
        }
        applyIndexes(db[spec.name], spec);
    }
}

void Database::applyIndexes(mongocxx::collection coll, const CollectionSpec& spec) {
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::document;

    for (const IndexSpec& index : spec.indexes) {
        document keys;
        for (std::size_t i = 0; i < index.fields.size(); ++i) {
            // kvp() requires a bsoncxx string_view, not a raw pointer.
            keys.append(kvp(bsoncxx::stdx::string_view(index.fields[i]),
                            index.directions[i]));
        }

        mongocxx::options::index options;
        if (index.unique) {
            options.unique(true);
        }
        try {
            coll.create_index(keys.extract(), options);
        } catch (const std::exception&) {
            // The index may already exist with the same key pattern.
        }
    }
}

} // namespace db