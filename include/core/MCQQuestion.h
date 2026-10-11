// OOP: Inheritance (Concrete Question Type)
#pragma once
#include "core/Question.h"
#include <vector>
#include <string>

namespace core {

class MCQQuestion : public Question {
public:
    MCQQuestion(QuestionId id, std::string subject, std::string topic,
                Difficulty difficulty, std::string text,
                std::vector<std::string> options, int correctIndex,
                std::string explanation);
    
    std::string display() const override;
    EvaluationResult evaluate(const std::string& answer) const override;
    std::string type() const override { return "MCQ"; }
    
    const std::vector<std::string>& options() const { return options_; }
    int correctIndex() const { return correctIndex_; }
    
private:
    std::vector<std::string> options_;
    int correctIndex_;
};

} // namespace core