// OOP: Inheritance (Concrete Question Type)
#pragma once
#include "core/Question.h"
#include <vector>
#include <string>

namespace core {

class TextQuestion : public Question {
public:
    TextQuestion(QuestionId id, std::string subject, std::string topic,
                 Difficulty difficulty, std::string text,
                 std::vector<std::string> keywords, int minKeywords,
                 std::string explanation);
    
    std::string display() const override;
    EvaluationResult evaluate(const std::string& answer) const override;
    std::string type() const override { return "Text"; }
    
    const std::vector<std::string>& keywords() const { return keywords_; }
    int minKeywords() const { return minKeywords_; }
    
private:
    std::vector<std::string> keywords_;
    int minKeywords_;
};

} // namespace core