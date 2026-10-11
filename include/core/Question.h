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

// Which side of the skill tree a question trains.
enum class SkillKind {
    Hard, // technical: algorithms, SQL, circuit analysis...
    Soft  // behavioural: communication, teamwork, conflict...
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

    // What the solver is being asked to demonstrate, shown above the prompt.
    const std::string& objective() const { return objective_; }
    void setObjective(std::string objective) { objective_ = std::move(objective); }

    SkillKind skillKind() const { return skillKind_; }
    void setSkillKind(SkillKind kind) { skillKind_ = kind; }

    const std::vector<std::string>& skillTags() const { return skillTags_; }
    void setSkillTags(std::vector<std::string> tags) { skillTags_ = std::move(tags); }
    // Words highlighted in the prompt and expected in the answer.
    const std::vector<std::string>& keywords() const { return keywords_; }
    void setKeywords(std::vector<std::string> keywords) { keywords_ = std::move(keywords); }

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
    std::string objective_;
    SkillKind skillKind_ = SkillKind::Hard;
    std::vector<std::string> skillTags_;
    std::vector<std::string> keywords_;
};

using QuestionPtr = std::shared_ptr<Question>;

} // namespace core
