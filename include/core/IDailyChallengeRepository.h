// OOP: Interface Segregation (Repository Contract)
#pragma once
#include "core/Question.h"
#include "core/StrongId.h"
#include "core/Optional.h"
#include <chrono>

namespace core {

struct DailyChallenge {
    std::string courseId;
    std::chrono::system_clock::time_point date;
    QuestionId questionId;
    int bonusPoints = 10;
};

class IDailyChallengeRepository {
public:
    virtual ~IDailyChallengeRepository() = default;
    virtual Optional<DailyChallenge> findByCourseAndDate(const std::string& courseId, 
        std::chrono::system_clock::time_point date) = 0;
    virtual void save(const DailyChallenge& challenge) = 0;
    virtual std::vector<DailyChallenge> findByDateRange(
        std::chrono::system_clock::time_point from,
        std::chrono::system_clock::time_point to) = 0;
};

} // namespace core