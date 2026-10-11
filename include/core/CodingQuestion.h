// OOP: Inheritance (Concrete Question Type)
#pragma once
#include "core/Question.h"
#include <string>

namespace core {

struct CodingTestCase {
    std::string input;
    std::string expectedOutput;
    bool isHidden;
};

class CodingQuestion : public Question {
public:
    CodingQuestion(QuestionId id, std::string subject, std::string topic,
                   Difficulty difficulty, std::string text,
                   std::string templateCode, std::vector<CodingTestCase> testCases,
                   std::string explanation);
    
    std::string display() const override;
    EvaluationResult evaluate(const std::string& answer) const override;
    std::string type() const override { return "Coding"; }
    
    const std::string& templateCode() const { return templateCode_; }
    const std::vector<CodingTestCase>& testCases() const { return testCases_; }
    
private:
    std::string templateCode_;
    std::vector<CodingTestCase> testCases_;
};

} // namespace core