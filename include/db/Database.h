// OOP: Singleton Pattern (Database Connection Manager)
#pragma once
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>
#include <string>
#include <memory>
#include <mutex>
#include <vector>

namespace db {

// Schema description used when idempotently creating collections and indexes.
// Declared at namespace scope so helper builders can construct it without
// needing access to Database's private section.
struct IndexSpec {
    const char* name;
    std::vector<const char*> fields;
    std::vector<int> directions;
    bool unique = false;
};

struct CollectionSpec {
    const char* name;
    std::vector<IndexSpec> indexes;
};

IndexSpec makeIndex(const char* name,
                    std::vector<const char*> fields,
                    std::vector<int> directions,
                    bool unique);

CollectionSpec makeCollection(const char* name, std::vector<IndexSpec> indexes);

class Database {
public:
    static Database& instance() {
        static Database instance;
        return instance;
    }
    
    // Non-copyable, non-movable
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    Database(Database&&) = delete;
    Database& operator=(Database&&) = delete;
    
    // Initialize with URI and database name
    void initialize(const std::string& uri, const std::string& dbName);
    
    // Get a client from the pool (RAII)
    mongocxx::pool::entry acquire();
    
    // Get database name
    const std::string& databaseName() const { return dbName_; }
    
    // Check if initialized
    bool isInitialized() const { return initialized_; }
    
    // Ping the database
    bool ping();
    
    // Create collections and their indexes (idempotent)
    void createCollections();
    
private:
    Database() = default;
    ~Database() = default;
    
    void applyIndexes(mongocxx::collection coll, const CollectionSpec& spec);
    static std::vector<CollectionSpec> collectionSpecs();
    
    std::unique_ptr<mongocxx::instance> instance_;
    std::unique_ptr<mongocxx::pool> pool_;
    std::string uri_;
    std::string dbName_;
    bool initialized_ = false;
    std::mutex initMutex_;
};

} // namespace db