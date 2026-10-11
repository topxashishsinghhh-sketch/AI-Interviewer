// OOP: Repository Pattern (MongoDB Implementation)
#include "db/MongoQuestionRepository.h"
#include "db/Database.h"
#include "db/BSONMappers.h"
#include "db/DatabaseException.h"
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <mongocxx/exception/bulk_write_exception.hpp>

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;

namespace db {

MongoQuestionRepository::MongoQuestionRepository(const std::string& dbName)
    : dbName_(dbName) {}

core::QuestionPtr MongoQuestionRepository::findById(const core::QuestionId& id) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["questions"];
        
        auto filter = document{} << "_id" << id.get() << finalize;
        auto result = coll.find_one(filter.view());
        
        if (result) {
            return QuestionMapper::fromBSON(result->view());
        }
        return nullptr;
    } catch (const std::exception& e) {
        throw QueryException(std::string("findById failed: ") + e.what());
    }
}

std::vector<core::QuestionPtr> MongoQuestionRepository::findAll() {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["questions"];
        
        std::vector<core::QuestionPtr> questions;
        for (auto&& doc : coll.find({})) {
            auto q = QuestionMapper::fromBSON(doc);
            if (q) questions.push_back(q);
        }
        return questions;
    } catch (const std::exception& e) {
        throw QueryException(std::string("findAll failed: ") + e.what());
    }
}

std::vector<core::QuestionPtr> MongoQuestionRepository::findBySubject(const std::string& subject) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["questions"];
        
        auto filter = document{} << "subject" << subject << finalize;
        std::vector<core::QuestionPtr> questions;
        for (auto&& doc : coll.find(filter.view())) {
            auto q = QuestionMapper::fromBSON(doc);
            if (q) questions.push_back(q);
        }
        return questions;
    } catch (const std::exception& e) {
        throw QueryException(std::string("findBySubject failed: ") + e.what());
    }
}

std::vector<core::QuestionPtr> MongoQuestionRepository::findBySubjectAndDifficulty(
    const std::string& subject, core::Difficulty difficulty) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["questions"];
        
        auto filter = document{} 
            << "subject" << subject 
            << "difficulty" << core::toString(difficulty) 
            << finalize;
        
        std::vector<core::QuestionPtr> questions;
        for (auto&& doc : coll.find(filter.view())) {
            auto q = QuestionMapper::fromBSON(doc);
            if (q) questions.push_back(q);
        }
        return questions;
    } catch (const std::exception& e) {
        throw QueryException(std::string("findBySubjectAndDifficulty failed: ") + e.what());
    }
}

std::vector<core::QuestionPtr> MongoQuestionRepository::findByTopic(const std::string& topic) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["questions"];
        
        auto filter = document{} << "topic" << topic << finalize;
        std::vector<core::QuestionPtr> questions;
        for (auto&& doc : coll.find(filter.view())) {
            auto q = QuestionMapper::fromBSON(doc);
            if (q) questions.push_back(q);
        }
        return questions;
    } catch (const std::exception& e) {
        throw QueryException(std::string("findByTopic failed: ") + e.what());
    }
}

void MongoQuestionRepository::save(const core::QuestionPtr& question) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["questions"];
        
        auto doc = QuestionMapper::toBSON(question);
        
        auto filter = document{} << "_id" << question->id().get() << finalize;
        auto options = mongocxx::options::replace{}.upsert(true);
        coll.replace_one(filter.view(), doc.view(), options);
    } catch (const std::exception& e) {
        throw QueryException(std::string("save failed: ") + e.what());
    }
}

void MongoQuestionRepository::remove(const core::QuestionId& id) {
    try {
        auto client = Database::instance().acquire();
        auto coll = (*client)[dbName_]["questions"];
        
        auto filter = document{} << "_id" << id.get() << finalize;
        coll.delete_one(filter.view());
    } catch (const std::exception& e) {
        throw QueryException(std::string("remove failed: ") + e.what());
    }
}

bool MongoQuestionRepository::exists(const core::QuestionId& id) {
    return findById(id) != nullptr;
}

} // namespace db