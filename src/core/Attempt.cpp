// OOP: Value Object (Immutable Data Carrier)
#include "core/Attempt.h"
#include "core/StrongId.h"
#include <chrono>
#include <utility>

namespace core {

// Parameters use a trailing underscore so they do not shadow the members they
// initialise (keeps -Wshadow clean).
Attempt::Attempt(AttemptId id_, UserId userId_, QuestionId questionId_,
                 std::string answer_, bool correct_, double score_,
                 Difficulty difficulty_, int timeTakenSeconds_)
    : id(std::move(id_)), userId(std::move(userId_)), questionId(std::move(questionId_)),
      answer(std::move(answer_)), correct(correct_), score(score_),
      difficulty(difficulty_), timeTakenSeconds(timeTakenSeconds_),
      timestamp(std::chrono::system_clock::now()) {}

} // namespace core