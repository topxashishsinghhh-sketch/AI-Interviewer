// OOP: Facade Pattern (UI Entry Point)
#pragma once
#include "auth/AuthService.h"
#include "admin/AdminService.h"
#include "course/CourseService.h"
#include "progress/ProgressService.h"
#include "interview/InterviewService.h"
#include "interview/SessionFactory.h"
#include "core/FeedbackGenerator.h"
#include "core/RuleBasedFeedback.h"
#include "core/QuestionSelectionStrategy.h"
#include "core/LinearRampStrategy.h"
#include "core/AdaptiveStrategy.h"
#include "core/IScoreboardRepository.h"
#include "core/IDailyChallengeRepository.h"
#include "db/RepositoryFactory.h"
#include <memory>
#include <string>

namespace ui {

class ConsoleUI {
public:
    ConsoleUI(int argc = 0, char* argv[] = nullptr);
    int run(int argc, char* argv[]);
    
private:
    void showHelp();
    void showVersion();
    bool handleRegister();
    bool handleLogin();
    void showDashboard(const core::UserPtr& user);
    void showAdminDashboard(const core::UserPtr& user);
    void handleCourseSelection(const core::UserPtr& user);
    void handleSubjectSelection(const core::UserPtr& user, const std::string& courseId);
    void handleModeSelection(const core::UserPtr& user, const std::string& courseId, const std::string& subjectId);
    void handleInterview(const core::UserPtr& user, const std::string& subject, const std::string& topic, core::InterviewSession::Mode mode);
    void handleDailyChallenge(const core::UserPtr& user, const std::string& courseId);
    void showProfile(const core::UserPtr& user);
    void showLeaderboard(const core::UserPtr& user);
    void showStreakCalendar(const core::UserPtr& user);
    void finalizeSession(const core::UserPtr& user, const core::FeedbackReport& report);
    void showAdminPlatformStats();
    void showAdminUsers();
    void showAdminQuestionStats();
    void promptContinue();
    
    std::unique_ptr<auth::AuthService> authService_;
    std::unique_ptr<admin::AdminService> adminService_;
    std::shared_ptr<course::CourseService> courseService_;
    std::unique_ptr<progress::ProgressService> progressService_;
    std::shared_ptr<core::IUserRepository> userRepo_;
    std::shared_ptr<core::IQuestionRepository> questionRepo_;
    std::shared_ptr<core::IAttemptRepository> attemptRepo_;
    std::shared_ptr<core::IScoreboardRepository> scoreboardRepo_;
    std::shared_ptr<core::IDailyChallengeRepository> dailyChallengeRepo_;
    std::shared_ptr<core::FeedbackGenerator> feedbackGen_;
    std::shared_ptr<core::QuestionSelectionStrategy> strategy_;
    core::UserPtr currentUser_;
    bool explainMode_ = false;
};

} // namespace ui