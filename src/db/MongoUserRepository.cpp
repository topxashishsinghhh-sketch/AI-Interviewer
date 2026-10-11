// OOP: Repository Pattern (MongoDB Implementation)
#include "db/MongoUserRepository.h"
#include "db/Database.h"
#include "db/BSONMappers.h"
#include "db/DatabaseException.h"
#include <bsoncxx/builder/stream/document.hpp>
#include <mongocxx/exception/bulk_write_exception.hpp>
#include <mongocxx/exception/operation_exception.hpp>

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;

namespace db {

MongoUserRepository::MongoUserRepository(const std::string& dbName)
    : dbName_(dbName) {}

core::UserPtr MongoUserRepository::findById(const core::UserId& id) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["users"];
        
        auto filter = document{} << "_id" << id.get() << finalize;
        auto result = coll.find_one(filter.view());
        
        if (result) {
            return UserMapper::fromBSON(result->view());
        }
        return nullptr;
    } catch (const std::exception& e) {
        throw QueryException(std::string("findById failed: ") + e.what());
    }
}

core::UserPtr MongoUserRepository::findByEmail(const std::string& email) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["users"];
        
        auto filter = document{} << "email" << email << finalize;
        auto result = coll.find_one(filter.view());
        
        if (result) {
            return UserMapper::fromBSON(result->view());
        }
        return nullptr;
    } catch (const std::exception& e) {
        throw QueryException(std::string("findByEmail failed: ") + e.what());
    }
}

std::vector<core::UserPtr> MongoUserRepository::findAll() {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["users"];
        
        std::vector<core::UserPtr> users;
        for (auto&& doc : coll.find({})) {
            // The cursor yields bsoncxx::document::view, which is what the
            // mappers accept; there is no separate .view() accessor.
            users.push_back(UserMapper::fromBSON(doc));
        }
        return users;
    } catch (const std::exception& e) {
        throw QueryException(std::string("findAll failed: ") + e.what());
    }
}

void MongoUserRepository::save(const core::UserPtr& user) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["users"];
        
        auto doc = UserMapper::toBSON(user);
        
        // Upsert
        auto filter = document{} << "_id" << user->id().get() << finalize;
        auto options = mongocxx::options::replace{}.upsert(true);
        coll.replace_one(filter.view(), doc.view(), options);
    } catch (const mongocxx::bulk_write_exception& e) {
        if (isDuplicateKeyError(e.what(), static_cast<int>(e.code().value()))) {
            throw DuplicateKeyException("User with this email already exists");
        }
        throw QueryException(std::string("save failed: ") + e.what());
    } catch (const std::exception& e) {
        throw QueryException(std::string("save failed: ") + e.what());
    }
}

void MongoUserRepository::remove(const core::UserId& id) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["users"];
        
        auto filter = document{} << "_id" << id.get() << finalize;
        coll.delete_one(filter.view());
    } catch (const std::exception& e) {
        throw QueryException(std::string("remove failed: ") + e.what());
    }
}

bool MongoUserRepository::exists(const std::string& email) {
    return findByEmail(email) != nullptr;
}

} // namespace db