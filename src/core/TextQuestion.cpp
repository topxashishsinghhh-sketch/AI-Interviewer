// OOP: Inheritance (Concrete Question Type)
#include "core/TextQuestion.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace core {

TextQuestion::TextQuestion(QuestionId id, std::string subject, std::string topic,
                           Difficulty difficulty, std::string text,
                           std::vector<std::string> keywords, int minKeywords,
                           std::string explanation)
    : Question(std::move(id), std::move(subject), std::move(topic), difficulty,
               std::move(text), std::move(explanation)),
      keywords_(std::move(keywords)), minKeywords_(minKeywords) {}

std::string TextQuestion::display() const {
    std::ostringstream oss;
    oss << text_ << "\n";
    oss << "(Enter your answer in free text. Keywords: ";
    for (size_t i = 0; i < keywords_.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << keywords_[i];
    }
    oss << ")\n";
    return oss.str();
}

static std::string toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

EvaluationResult TextQuestion::evaluate(const std::string& answer) const {
    EvaluationResult result;
    result.correct = false;
    result.score = 0.0;
    
    if (answer.empty()) {
        result.feedback = "No answer provided.";
        return result;
    }
    
    std::string lowerAnswer = toLower(answer);
    int matched = 0;
    
    for (const auto& keyword : keywords_) {
        if (lowerAnswer.find(toLower(keyword)) != std::string::npos) {
            matched++;
        }
    }
    
    if (matched >= minKeywords_) {
        result.correct = true;
        result.score = 1.0;
        result.feedback = "Good answer! " + explanation_;
    } else {
        result.score = std::min(1.0, static_cast<double>(matched) / minKeywords_);
        result.feedback = "Partial match (" + std::to_string(matched) + "/" + std::to_string(minKeywords_) + " keywords). " + explanation_;
    }
    
    return result;
}

} // namespace core