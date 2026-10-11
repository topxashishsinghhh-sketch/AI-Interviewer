// OOP: Service Layer (Admin Business Logic)
#pragma once
#include "core/IUserRepository.h"
#include "core/IQuestionRepository.h"
#include "core/IAttemptRepository.h"
#include "core/User.h"
#include <memory>
#include <string>
#include <vector>

namespace admin {

struct PlatformStats {
    int totalUsers = 0;
    int students = 0;
    int admins = 0;
    int totalQuestions = 0;
    int totalAttempts = 0;
    double averageScore = 0.0;
    int activeStreaks = 0;
};

struct UserSummary {
    core::UserPtr user;
    int solved = 0;
    int attempts = 0;
    int streak = 0;
};

struct QuestionStats {
    std::string subject;
    int easy = 0;
    int medium = 0;
    int hard = 0;
    int total = 0;
};

class AdminService {
public:
    AdminService(std::shared_ptr<core::IUserRepository> userRepo,
                 std::shared_ptr<core::IQuestionRepository> questionRepo,
                 std::shared_ptr<core::IAttemptRepository> attemptRepo);
    
    PlatformStats platformStats() const;
    std::vector<UserSummary> userSummaries() const;
    std::vector<QuestionStats> questionStats() const;
    
    bool removeUser(const core::UserId& id);
    bool resetStreak(const core::UserId& id);
    bool awardBadge(const core::UserId& id, const std::string& badge);
    
private:
    std::shared_ptr<core::IUserRepository> userRepo_;
    std::shared_ptr<core::IQuestionRepository> questionRepo_;
    std::shared_ptr<core::IAttemptRepository> attemptRepo_;
};

} // namespace admin