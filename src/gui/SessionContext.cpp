// OOP: Dependency Injection container (shared ownership for GUI screens)
#include "gui/SessionContext.h"
#include <cstdlib>

#ifdef USE_MONGO
#include "db/Database.h"
#include <cstdlib>

namespace {

// Reads an environment variable, returning fallback when it is unset/empty.
std::string envOrMongoVar(const char* name, const std::string& fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') return fallback;
    return std::string(value);
}

} // namespace
#endif

namespace gui {

SessionContext::SessionContext(std::string dataDir, db::StorageBackend backend, std::string mongoDbName)
    : dataDir_(std::move(dataDir))
    , backend_(backend)
    , mongoDbName_(std::move(mongoDbName)) {
    
    bool fellBack = false;
    
    if (backend_ == db::StorageBackend::Mongo) {
#ifdef USE_MONGO
        // The Database singleton must be initialised before any Mongo-backed
        // repository can acquire a client from the pool.
        try {
            db::Database::instance().initialize(
                envOrMongoVar("MONGODB_URI", "mongodb://localhost:27017"), mongoDbName_);
        } catch (const std::exception&) {
            fellBack = true;
        }
#else
        fellBack = true;
#endif
        if (!fellBack) {
            questionRepo_ = db::RepositoryFactory::createQuestionRepositoryWithFallback(dataDir_ + "/questions.json");
            attemptRepo_ = db::RepositoryFactory::createAttemptRepositoryWithFallback(dataDir_ + "/attempts.json");
            userRepo_ = db::RepositoryFactory::createUserRepositoryWithFallback(dataDir_ + "/users.json");
            if (!questionRepo_ || !attemptRepo_ || !userRepo_) {
                fellBack = true;
            }
        }
    }
    
    if (fellBack || backend_ == db::StorageBackend::Json) {
        backend_ = db::StorageBackend::Json;
        questionRepo_ = db::RepositoryFactory::createQuestionRepository(backend_, dataDir_ + "/questions.json");
        attemptRepo_ = db::RepositoryFactory::createAttemptRepository(backend_, dataDir_ + "/attempts.json");
        userRepo_ = db::RepositoryFactory::createUserRepository(backend_, dataDir_ + "/users.json");
    }
    
    // Progress repositories follow the selected backend so data is not split
    // across two stores; the fallback variants are used when JSON is in force.
    if (backend_ == db::StorageBackend::Mongo) {
#ifdef USE_MONGO
        scoreboardRepo_ = db::RepositoryFactory::createScoreboardRepository(
            backend_, dataDir_ + "/scoreboard.json");
        dailyChallengeRepo_ = db::RepositoryFactory::createDailyChallengeRepository(
            backend_, dataDir_ + "/daily_challenges.json");
#else
        scoreboardRepo_ = db::RepositoryFactory::createScoreboardRepositoryWithFallback(
            dataDir_ + "/scoreboard.json");
        dailyChallengeRepo_ = db::RepositoryFactory::createDailyChallengeRepositoryWithFallback(
            dataDir_ + "/daily_challenges.json");
#endif
    } else {
        scoreboardRepo_ = db::RepositoryFactory::createScoreboardRepositoryWithFallback(dataDir_ + "/scoreboard.json");
        dailyChallengeRepo_ = db::RepositoryFactory::createDailyChallengeRepositoryWithFallback(dataDir_ + "/daily_challenges.json");
    }
    
    courseService_ = std::make_shared<course::CourseService>();
    feedbackGen_ = std::make_shared<core::RuleBasedFeedback>();
    strategy_ = std::make_shared<core::LinearRampStrategy>();
    
    authService_ = std::make_unique<auth::AuthService>(userRepo_);
    adminService_ = std::make_unique<admin::AdminService>(userRepo_, questionRepo_, attemptRepo_);
    progressService_ = std::make_unique<progress::ProgressService>(
        userRepo_, questionRepo_, scoreboardRepo_, dailyChallengeRepo_, courseService_);
}

} // namespace gui
