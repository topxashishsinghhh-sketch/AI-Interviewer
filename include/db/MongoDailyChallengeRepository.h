// OOP: Repository Pattern (MongoDB Implementation)
#pragma once
#include "core/IDailyChallengeRepository.h"
#include "core/Question.h"
#include "core/StrongId.h"
#include "core/Optional.h"
#include <chrono>
#include <vector>
#include <string>

namespace db {

class MongoDailyChallengeRepository : public core::IDailyChallengeRepository {
public:
    explicit MongoDailyChallengeRepository(const std::string& dbName = "ai_interviewer");
    
    core::Optional<core::DailyChallenge> findByCourseAndDate(const std::string& courseId, 
        std::chrono::system_clock::time_point date) override;
    void save(const core::DailyChallenge& challenge) override;
    std::vector<core::DailyChallenge> findByDateRange(
        std::chrono::system_clock::time_point from,
        std::chrono::system_clock::time_point to) override;
    
private:
    std::string dbName_;
};

} // namespace db