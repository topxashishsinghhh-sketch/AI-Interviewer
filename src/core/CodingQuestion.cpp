// OOP: Inheritance (Concrete Question Type)
#include "core/CodingQuestion.h"
#include <sstream>
#include <string>

namespace core {

CodingQuestion::CodingQuestion(QuestionId id, std::string subject, std::string topic,
                               Difficulty difficulty, std::string text,
                               std::string templateCode, std::vector<CodingTestCase> testCases,
                               std::string explanation)
    : Question(std::move(id), std::move(subject), std::move(topic), difficulty,
               std::move(text), std::move(explanation)),
      templateCode_(std::move(templateCode)), testCases_(std::move(testCases)) {}

std::string CodingQuestion::display() const {
    std::ostringstream oss;
    oss << text_ << "\n\n";
    oss << "Template:\n```cpp\n" << templateCode_ << "\n```\n";
    oss << "\nTest cases: " << testCases_.size() << " (some hidden)\n";
    return oss.str();
}

EvaluationResult CodingQuestion::evaluate(const std::string& answer) const {
    EvaluationResult result;
    result.correct = false;
    result.score = 0.0;
    
    if (answer.empty()) {
        result.feedback = "No code submitted.";
        return result;
    }
    
    // Mock evaluation for compilation environments without g++
    // In production, this would compile and run the code
    result.feedback = "[Mock] Code submitted (" + std::to_string(answer.length()) + " chars). "
                      "Full compilation requires g++ in PATH. " + explanation_;
    result.score = 0.5; // Partial credit for submission
    result.correct = false;
    
    return result;
}

} // namespace core