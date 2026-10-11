// OOP: Repository Pattern (JSON Implementation)
#pragma once
#include "core/IDailyChallengeRepository.h"
#include <map>
#include <string>
#include <vector>

namespace db {

class JsonDailyChallengeRepository : public core::IDailyChallengeRepository {
public:
    explicit JsonDailyChallengeRepository(const std::string& filePath);
    
    core::Optional<core::DailyChallenge> findByCourseAndDate(const std::string& courseId, 
        std::chrono::system_clock::time_point date) override;
    void save(const core::DailyChallenge& challenge) override;
    std::vector<core::DailyChallenge> findByDateRange(
        std::chrono::system_clock::time_point from,
        std::chrono::system_clock::time_point to) override;

private:
    std::string filePath_;
    std::map<std::string, core::DailyChallenge> challenges_; // key: courseId_date
    void load();
    void save();
    std::string makeKey(const std::string& courseId, std::chrono::system_clock::time_point date) const;
};

} // namespace db