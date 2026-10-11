// OOP: Inheritance (Concrete Question Type)
#include "core/MCQQuestion.h"
#include <sstream>

namespace core {

MCQQuestion::MCQQuestion(QuestionId id, std::string subject, std::string topic,
                         Difficulty difficulty, std::string text,
                         std::vector<std::string> options, int correctIndex,
                         std::string explanation)
    : Question(std::move(id), std::move(subject), std::move(topic), difficulty,
               std::move(text), std::move(explanation)),
      options_(std::move(options)), correctIndex_(correctIndex) {}

std::string MCQQuestion::display() const {
    std::ostringstream oss;
    oss << text_ << "\n";
    for (size_t i = 0; i < options_.size(); ++i) {
        oss << "  " << char('A' + i) << ". " << options_[i] << "\n";
    }
    return oss.str();
}

EvaluationResult MCQQuestion::evaluate(const std::string& answer) const {
    EvaluationResult result;
    result.correct = false;
    result.score = 0.0;
    
    if (answer.empty()) {
        result.feedback = "No answer provided.";
        return result;
    }
    
    // Cast explicitly: std::toupper takes/returns int, and keeping the result an
    // int avoids the char conversion warning and comparison surprises.
    const int upperAnswer = std::toupper(static_cast<unsigned char>(answer[0]));
    int userIndex = upperAnswer - 'A';
    
    if (userIndex >= 0 && userIndex < static_cast<int>(options_.size())) {
        result.correct = (userIndex == correctIndex_);
        result.score = result.correct ? 1.0 : 0.0;
        result.feedback = result.correct 
            ? "Correct! " + explanation_
            : "Incorrect. The correct answer is " + std::string(1, static_cast<char>('A' + correctIndex_)) + ". " + explanation_;
    } else {
        result.feedback = "Invalid option. " + explanation_;
    }
    
    return result;
}

} // namespace core