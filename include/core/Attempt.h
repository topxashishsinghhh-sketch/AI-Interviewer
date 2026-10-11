// OOP: Value Object (Immutable Data Carrier)
#pragma once
#include "core/StrongId.h"
#include "core/Difficulty.h"
#include <string>
#include <chrono>
#include <ctime>
#include <cstdio>

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

    // Local calendar day, used for streak calendars and per-day activity.
    std::string timestampAsDateKey() const {
        const std::time_t seconds = std::chrono::system_clock::to_time_t(timestamp);
        const std::tm* tm = std::localtime(&seconds);
        if (tm == nullptr) return std::string();
        char buffer[16] = {0};
        std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d",
                      tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
        return std::string(buffer);
    }
};

} // namespace core