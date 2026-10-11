// OOP: Value Object (Immutable Data Carrier)
#pragma once
#include "core/StrongId.h"
#include "core/Difficulty.h"
#include <string>
#include <chrono>

namespace core {

struct Attempt {
    AttemptId id;
    UserId userId;
    QuestionId questionId;
    std::string answer;
    bool correct;
    double score;
    Difficulty difficulty;
    int timeTakenSeconds;
    std::chrono::system_clock::time_point timestamp;
    bool bookmarked = false;
    std::string feedback;
    
    Attempt() = default;
    Attempt(AttemptId id, UserId userId, QuestionId questionId,
            std::string answer, bool correct, double score,
            Difficulty difficulty, int timeTakenSeconds);
};

} // namespace core