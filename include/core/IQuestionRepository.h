// OOP: Interface Segregation (Repository Contract)
#pragma once
#include "core/Question.h"
#include "core/StrongId.h"
#include "core/Difficulty.h"
#include <vector>

namespace core {

class IQuestionRepository {
public:
    virtual ~IQuestionRepository() = default;
    virtual QuestionPtr findById(const QuestionId& id) = 0; // Returns nullptr if not found
    virtual std::vector<QuestionPtr> findAll() = 0;
    virtual std::vector<QuestionPtr> findBySubject(const std::string& subject) = 0;
    virtual std::vector<QuestionPtr> findBySubjectAndDifficulty(
        const std::string& subject, Difficulty difficulty) = 0;
    virtual std::vector<QuestionPtr> findByTopic(const std::string& topic) = 0;
    virtual void save(const QuestionPtr& question) = 0;
    virtual void remove(const QuestionId& id) = 0;
    virtual bool exists(const QuestionId& id) = 0;
};

} // namespace core