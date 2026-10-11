// OOP: Repository Pattern (JSON Implementation)
#pragma once
#include "core/IQuestionRepository.h"
#include "core/Question.h"
#include "core/MCQQuestion.h"
#include "core/TextQuestion.h"
#include "core/CodingQuestion.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <fstream>
#include "json.hpp"

namespace db {

class JsonQuestionRepository : public core::IQuestionRepository {
public:
    explicit JsonQuestionRepository(const std::string& filePath);
    
    core::QuestionPtr findById(const core::QuestionId& id) override; // Returns nullptr if not found
    std::vector<core::QuestionPtr> findAll() override;
    std::vector<core::QuestionPtr> findBySubject(const std::string& subject) override;
    std::vector<core::QuestionPtr> findBySubjectAndDifficulty(
        const std::string& subject, core::Difficulty difficulty) override;
    std::vector<core::QuestionPtr> findByTopic(const std::string& topic) override;
    void save(const core::QuestionPtr& question) override;
    void remove(const core::QuestionId& id) override;
    bool exists(const core::QuestionId& id) override;
    
private:
    std::string filePath_;
    std::unordered_map<std::string, core::QuestionPtr> cache_;
    nlohmann::json load();
    void save(const nlohmann::json& data);
    core::QuestionPtr deserialize(const nlohmann::json& j);
    nlohmann::json serialize(const core::QuestionPtr& question);
};

} // namespace db