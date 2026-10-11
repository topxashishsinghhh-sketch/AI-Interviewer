// OOP: Abstraction (Abstract Base Class)
#include "core/Question.h"
#include "core/StrongId.h"

namespace core {

Question::Question(QuestionId id, std::string subject, std::string topic,
                   Difficulty difficulty, std::string text, std::string explanation)
    : id_(std::move(id)), subject_(std::move(subject)), topic_(std::move(topic)),
      difficulty_(difficulty), text_(std::move(text)), explanation_(std::move(explanation)) {}

} // namespace core