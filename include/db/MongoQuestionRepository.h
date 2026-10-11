// OOP: Repository Pattern (MongoDB Implementation)
#pragma once
#include "core/IQuestionRepository.h"
#include "core/Question.h"
#include <string>
#include <vector>
#include <memory>

namespace db {

class MongoQuestionRepository : public core::IQuestionRepository {
public:
    explicit MongoQuestionRepository(const std::string& dbName = "ai_interviewer");
    
    core::QuestionPtr findById(const core::QuestionId& id) override;
    std::vector<core::QuestionPtr> findAll() override;
    std::vector<core::QuestionPtr> findBySubject(const std::string& subject) override;
    std::vector<core::QuestionPtr> findBySubjectAndDifficulty(
        const std::string& subject, core::Difficulty difficulty) override;
    std::vector<core::QuestionPtr> findByTopic(const std::string& topic) override;
    void save(const core::QuestionPtr& question) override;
    void remove(const core::QuestionId& id) override;
    bool exists(const core::QuestionId& id) override;
    
private:
    std::string dbName_;
};

} // namespace db