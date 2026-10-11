// OOP: Dependency Injection container (shared ownership for GUI screens)
#pragma once
#include "db/RepositoryFactory.h"
#include "auth/AuthService.h"
#include "admin/AdminService.h"
#include "progress/ProgressService.h"
#include "course/CourseService.h"
#include "core/IUserRepository.h"
#include "core/IQuestionRepository.h"
#include "core/IAttemptRepository.h"
#include "core/IScoreboardRepository.h"
#include "core/IDailyChallengeRepository.h"
#include "core/FeedbackGenerator.h"
#include "core/RuleBasedFeedback.h"
#include "core/QuestionSelectionStrategy.h"
#include "core/LinearRampStrategy.h"
#include <memory>
#include <string>

namespace gui {

// Owns the repositories and services every GUI screen needs. Screens receive a
// const pointer and never own or construct services themselves.
class SessionContext {
public:
    explicit SessionContext(std::string dataDir = "./data",
                            db::StorageBackend backend = db::StorageBackend::Json,
                            std::string mongoDbName = "ai_interviewer");
    
    db::StorageBackend backend() const { return backend_; }
    const std::string& dataDir() const { return dataDir_; }
    
    auth::AuthService& auth() { return *authService_; }
    admin::AdminService& admin() { return *adminService_; }
    progress::ProgressService& progress() { return *progressService_; }
    course::CourseService& courses() { return *courseService_; }
    
    std::shared_ptr<core::IQuestionRepository> questionRepo() { return questionRepo_; }
    std::shared_ptr<core::IAttemptRepository> attemptRepo() { return attemptRepo_; }
    std::shared_ptr<core::IUserRepository> userRepo() { return userRepo_; }
    std::shared_ptr<core::FeedbackGenerator> feedbackGenerator() { return feedbackGen_; }
    std::shared_ptr<core::IScoreboardRepository> scoreboardRepo() { return scoreboardRepo_; }
    std::shared_ptr<core::IDailyChallengeRepository> dailyChallengeRepo() { return dailyChallengeRepo_; }
    
private:
    std::string dataDir_;
    db::StorageBackend backend_;
    std::string mongoDbName_;
    
    std::shared_ptr<core::IUserRepository> userRepo_;
    std::shared_ptr<core::IQuestionRepository> questionRepo_;
    std::shared_ptr<core::IAttemptRepository> attemptRepo_;
    std::shared_ptr<core::IScoreboardRepository> scoreboardRepo_;
    std::shared_ptr<core::IDailyChallengeRepository> dailyChallengeRepo_;
    
    std::shared_ptr<course::CourseService> courseService_;
    std::shared_ptr<core::FeedbackGenerator> feedbackGen_;
    std::shared_ptr<core::QuestionSelectionStrategy> strategy_;
    std::unique_ptr<auth::AuthService> authService_;
    std::unique_ptr<admin::AdminService> adminService_;
    std::unique_ptr<progress::ProgressService> progressService_;
};

} // namespace gui
