// OOP: Abstraction (Abstract Base Class)
#pragma once
#include "core/Difficulty.h"
#include "core/StrongId.h"
#include <string>
#include <vector>
#include <memory>

namespace core {

struct EvaluationResult {
    bool correct;
    double score; // 0.0 to 1.0
    std::string feedback;
};

class Question {
public:
    Question(QuestionId id, std::string subject, std::string topic, 
             Difficulty difficulty, std::string text, std::string explanation);
    virtual ~Question() = default;
    
    const QuestionId& id() const { return id_; }
    const std::string& subject() const { return subject_; }
    const std::string& topic() const { return topic_; }
    Difficulty difficulty() const { return difficulty_; }
    const std::string& text() const { return text_; }
    const std::string& explanation() const { return explanation_; }
    
    virtual std::string display() const = 0;
    virtual EvaluationResult evaluate(const std::string& answer) const = 0;
    virtual std::string type() const = 0;
    
protected:
    QuestionId id_;
    std::string subject_;
    std::string topic_;
    Difficulty difficulty_;
    std::string text_;
    std::string explanation_;
};

using QuestionPtr = std::shared_ptr<Question>;

} // namespace core