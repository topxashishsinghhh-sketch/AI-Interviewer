// OOP: Facade Pattern (HTTP transport over the service layer)
#pragma once
#include "web/SessionStore.h"
#include "web/RequestContext.h"
#include "web/JsonApi.h"
#include "auth/AuthService.h"
#include "admin/AdminService.h"
#include "course/CourseService.h"
#include "theory/TheoryService.h"
#include "progress/ProgressService.h"
#include "problem/ProblemService.h"
#include "skills/SkillsService.h"
#include "db/RepositoryFactory.h"
#include "httplib.h"

#include <memory>
#include <string>

namespace core {
    class InterviewSession;
}

namespace web {

using nlohmann::json;

// Owns the HTTP server and the services the endpoints call. No business logic
// lives here: every decision is delegated to auth/admin/course/progress, and
// every read and write goes through a repository interface.
class HttpServer {
public:
    struct Config {
        std::string host = "127.0.0.1";
        int port = 8080;
        std::string dataDir = "./data";
        std::string staticDir = "./web";
        db::StorageBackend backend = db::StorageBackend::Json;
        std::string dbName = "ai_interviewer";
        std::string uri = "mongodb://localhost:27017";
    };

    explicit HttpServer(const Config& config);
    ~HttpServer();

    bool start();
    void stop();
    bool listening() const { return listening_; }

private:
    void registerRoutes();

    // Auth middleware: resolves the bearer token into a RequestContext.
    RequestContext authenticate(const httplib::Request& request) const;

    // Helpers shared by handlers.
    static void sendJson(httplib::Response& response, int status, const json& body);
    static void sendError(httplib::Response& response, int status, const std::string& message);
    static bool requireAdmin(const RequestContext& context, httplib::Response& response);
    static bool requireAuth(const RequestContext& context, httplib::Response& response);

    Config config_;
    std::unique_ptr<httplib::Server> server_;
    bool listening_ = false;

    // Services and repositories, created once at startup.
    std::shared_ptr<core::IUserRepository> userRepo_;
    std::shared_ptr<core::IQuestionRepository> questionRepo_;
    std::shared_ptr<core::IAttemptRepository> attemptRepo_;
    std::shared_ptr<core::IScoreboardRepository> scoreboardRepo_;
    std::shared_ptr<core::IDailyChallengeRepository> dailyChallengeRepo_;
    std::shared_ptr<course::CourseService> courseService_;
    std::unique_ptr<theory::TheoryService> theoryService_;
    std::unique_ptr<auth::AuthService> authService_;
    std::unique_ptr<admin::AdminService> adminService_;
    std::unique_ptr<progress::ProgressService> progressService_;
    std::unique_ptr<problem::ProblemService> problemService_;
    std::unique_ptr<skills::SkillsService> skillsService_;
    std::shared_ptr<core::FeedbackGenerator> feedbackGen_;
};

} // namespace web
