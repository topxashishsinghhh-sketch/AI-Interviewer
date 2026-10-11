// OOP: Service Layer (Progress and leaderboard business logic)
#pragma once
#include "core/IUserRepository.h"
#include "core/IScoreboardRepository.h"
#include "core/IDailyChallengeRepository.h"
#include "core/IQuestionRepository.h"
#include "core/FeedbackGenerator.h"
#include "core/User.h"
#include "course/CourseService.h"
#include "core/Optional.h"
#include <memory>
#include <string>
#include <vector>

namespace progress {

struct LeaderboardQuery {
    std::string scope = "global";
    std::string window = "alltime";
    std::string ranking = "score";
    int topN = 10;
};

class ProgressService {
public:
    ProgressService(std::shared_ptr<core::IUserRepository> userRepo,
                    std::shared_ptr<core::IQuestionRepository> questionRepo,
                    std::shared_ptr<core::IScoreboardRepository> scoreboardRepo,
                    std::shared_ptr<core::IDailyChallengeRepository> dailyChallengeRepo,
                    std::shared_ptr<course::CourseService> courseService);
    
    // Records activity, updates cumulative stats/badges, persists the user and
    // upserts the leaderboard entry. Returns the updated streak.
    int finalizeSession(const core::UserPtr& user, const core::FeedbackReport& report);
    
    // Deterministic per-course/per-day challenge (created on first request).
    core::Optional<core::DailyChallenge> ensureDailyChallenge(const std::string& courseId);
    core::Optional<core::DailyChallenge> dailyChallengeForToday(const std::string& courseId);
    
    std::vector<core::ScoreboardEntry> leaderboard(const LeaderboardQuery& query) const;
    int userRank(const core::UserId& userId, const LeaderboardQuery& query) const;
    core::Optional<core::ScoreboardEntry> userEntry(const core::UserId& userId,
                                                    const LeaderboardQuery& query) const;
    
    // Streak calendar helpers
    std::string todayKey() const;
    static bool isConsecutive(const std::string& fromKey, const std::string& toKey);
    
private:
    std::shared_ptr<core::IUserRepository> userRepo_;
    std::shared_ptr<core::IQuestionRepository> questionRepo_;
    std::shared_ptr<core::IScoreboardRepository> scoreboardRepo_;
    std::shared_ptr<core::IDailyChallengeRepository> dailyChallengeRepo_;
    std::shared_ptr<course::CourseService> courseService_;
};

} // namespace progress
