// OOP: Facade Pattern (HTTP transport over the service layer)
#include "web/HttpServer.h"
#include "core/InterviewSession.h"
#include "core/RuleBasedFeedback.h"
#include "core/LinearRampStrategy.h"
#include "core/AdaptiveStrategy.h"
#include "core/Student.h"
#include "core/CodingQuestion.h"
#include "problem/ProblemService.h"
#include "interview/InterviewService.h"
#include "interview/SessionFactory.h"
#ifdef USE_MONGO
#include "db/Database.h"
#endif

#include <chrono>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

using web::HttpServer;

namespace web {

namespace {

std::string tokenFromRequest(const httplib::Request& request) {
    // Accepts "Authorization: Bearer <token>" and the X-Auth-Token header,
    // which is easier to use from a browser's fetch().
    const std::string auth = request.get_header_value("Authorization");
    if (auth.rfind("Bearer ", 0) == 0 && auth.size() > 7) {
        return auth.substr(7);
    }
    return request.get_header_value("X-Auth-Token");
}

std::string stringField(const json& body, const char* key, const std::string& fallback = "") {
    if (!body.contains(key)) return fallback;
    const json& value = body.at(key);
    if (value.is_string()) return value.get<std::string>();
    if (value.is_number_integer()) {
        std::ostringstream oss;
        oss << value.get<long long>();
        return oss.str();
    }
    return fallback;
}

int intField(const json& body, const char* key, int fallback) {
    if (!body.contains(key)) return fallback;
    const json& value = body.at(key);
    if (value.is_number_integer()) return value.get<int>();
    if (value.is_number_float()) return static_cast<int>(value.get<double>());
    return fallback;
}

// cpp-httplib's get_param_value(name, default) overload does not exist; the
// two-argument form is get_param_value(name, hint), so defaults are applied
// here to keep the route bodies readable.
std::string paramOr(const httplib::Request& request, const std::string& name,
                    const std::string& fallback) {
    if (!request.has_param(name)) return fallback;
    const std::string value = request.get_param_value(name);
    return value.empty() ? fallback : value;
}

// Reads a non-negative "limit" query parameter. Anything unparseable or
// negative falls back to the caller's default so a typo cannot return an empty
// page or every row.
int parseLimit(const httplib::Request& request, int fallback) {
    if (!request.has_param("limit")) return fallback;
    const std::string raw = request.get_param_value("limit");
    if (raw.empty()) return fallback;
    std::istringstream iss(raw);
    int value = 0;
    if (!(iss >> value) || value < 0) return fallback;
    return value;
}

// ---- problem API payload helpers ----

json problemToJson(const core::QuestionPtr& question) {
    json out;
    if (!question) return out;
    out["id"] = question->id().get();
    out["subject"] = question->subject();
    out["topic"] = question->topic();
    out["difficulty"] = core::toString(question->difficulty());
    out["type"] = question->type();
    out["text"] = question->display();
    out["title"] = question->text();
    // Phase 11: objective, skill side, tags and the words to highlight.
    out["objective"] = question->objective();
    out["skillKind"] = question->skillKind() == core::SkillKind::Soft ? "Soft" : "Hard";
    out["skillTags"] = question->skillTags();

    // Keywords differ per question type: Text questions already own theirs,
    // everything else gets the topic words.
    std::vector<std::string> keywords = question->keywords();
    if (keywords.empty()) {
        std::string topic = question->topic();
        std::string current;
        for (std::size_t i = 0; i <= topic.size(); ++i) {
            if (i == topic.size() || topic[i] == '_' || topic[i] == ' ') {
                if (current.size() > 2) keywords.push_back(current);
                current.clear();
            } else {
                current += topic[i];
            }
        }
    }
    out["keywords"] = keywords;

    // Coding problems carry the starter code and the visible sample cases so the
    // editor and the "Run" button work without extra round trips.
    if (auto coding = std::dynamic_pointer_cast<core::CodingQuestion>(question)) {
        out["template"] = coding->templateCode();
        json samples = json::array();
        for (std::size_t i = 0; i < coding->testCases().size(); ++i) {
            const core::CodingTestCase& testCase = coding->testCases()[i];
            if (testCase.isHidden) continue;
            json item;
            item["input"] = testCase.input;
            item["expected"] = testCase.expectedOutput;
            samples.push_back(item);
        }
        out["samples"] = samples;
    }
    return out;
}

json submissionToJson(const problem::Submission& submission) {
    json out;
    out["id"] = submission.id.get();
    out["questionId"] = submission.questionId.get();
    out["answer"] = submission.answer;
    out["correct"] = submission.correct;
    out["score"] = submission.score;
    out["feedback"] = submission.feedback;
    out["difficulty"] = submission.difficulty;
    out["submittedAt"] = submission.submittedAt;
    return out;
}

json statsToJson(const problem::UserStats& stats) {
    json out;
    json solved;
    solved["easy"] = stats.solved.easy;
    solved["medium"] = stats.solved.medium;
    solved["hard"] = stats.solved.hard;
    solved["total"] = stats.solved.total();
    json available;
    available["easy"] = stats.available.easy;
    available["medium"] = stats.available.medium;
    available["hard"] = stats.available.hard;
    available["total"] = stats.available.total();
    out["solved"] = solved;
    out["available"] = available;
    out["attempts"] = stats.attempts;
    out["acceptancePermille"] = stats.acceptancePermille;
    out["streak"] = stats.streak;
    out["longestStreak"] = stats.longestStreak;
    out["badges"] = stats.badges;
    json bySubject = json::array();
    for (std::size_t i = 0; i < stats.bySubject.size(); ++i) {
        const std::string& name = stats.bySubject[i].first;
        const problem::DifficultyCount& counts = stats.bySubject[i].second;
        json item;
        item["subject"] = name;
        item["easy"] = counts.easy;
        item["medium"] = counts.medium;
        item["hard"] = counts.hard;
        item["total"] = counts.total();
        bySubject.push_back(item);
    }
    out["bySubject"] = bySubject;
    return out;
}

} // namespace

HttpServer::HttpServer(const Config& config)
    : config_(config) {
#ifdef USE_MONGO
    if (config_.backend == db::StorageBackend::Mongo) {
        try {
            db::Database::instance().initialize(config_.uri, config_.dbName);
        } catch (const std::exception& e) {
            std::cerr << "[web] MongoDB unavailable (" << e.what() << "), using JSON\n";
            config_.backend = db::StorageBackend::Json;
        }
    }
#endif

    userRepo_ = db::RepositoryFactory::createUserRepository(
        config_.backend, config_.dataDir + "/users.json", config_.dbName);
    questionRepo_ = db::RepositoryFactory::createQuestionRepository(
        config_.backend, config_.dataDir + "/questions.json", config_.dbName);
    attemptRepo_ = db::RepositoryFactory::createAttemptRepository(
        config_.backend, config_.dataDir + "/attempts.json", config_.dbName);
    scoreboardRepo_ = db::RepositoryFactory::createScoreboardRepository(
        config_.backend, config_.dataDir + "/scoreboard.json", config_.dbName);
    dailyChallengeRepo_ = db::RepositoryFactory::createDailyChallengeRepository(
        config_.backend, config_.dataDir + "/daily_challenges.json", config_.dbName);

    courseService_ = std::make_shared<course::CourseService>();
    theoryService_ = std::make_unique<theory::TheoryService>(config_.dataDir, questionRepo_);
    feedbackGen_ = std::make_shared<core::RuleBasedFeedback>();
    authService_ = std::make_unique<auth::AuthService>(userRepo_);
    adminService_ = std::make_unique<admin::AdminService>(userRepo_, questionRepo_, attemptRepo_);
    skillsService_ = std::make_unique<skills::SkillsService>(questionRepo_, attemptRepo_, userRepo_);
    progressService_ = std::make_unique<progress::ProgressService>(
        userRepo_, questionRepo_, scoreboardRepo_, dailyChallengeRepo_, courseService_);
    problemService_ = std::make_unique<problem::ProblemService>(
        questionRepo_, attemptRepo_, userRepo_, feedbackGen_);

    server_ = std::make_unique<httplib::Server>();
    registerRoutes();
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::sendJson(httplib::Response& response, int status, const nlohmann::json& body) {
    response.status = status;
    response.set_content(body.dump(), "application/json");
}

void HttpServer::sendError(httplib::Response& response, int status, const std::string& message) {
    sendJson(response, status, JsonApi::error(message, status));
}

bool HttpServer::requireAuth(const RequestContext& context, httplib::Response& response) {
    if (context.authenticated()) return true;
    sendError(response, 401, "Not authenticated");
    return false;
}

bool HttpServer::requireAdmin(const RequestContext& context, httplib::Response& response) {
    if (!context.authenticated()) {
        sendError(response, 401, "Not authenticated");
        return false;
    }
    if (!context.isAdmin()) {
        sendError(response, 403, "Admin privileges required");
        return false;
    }
    return true;
}

RequestContext HttpServer::authenticate(const httplib::Request& request) const {
    const std::string token = tokenFromRequest(request);
    if (token.empty()) return RequestContext(false, "", "", "");

    const core::UserPtr user = SessionStore::instance().userForToken(token);
    if (!user) return RequestContext(false, "", "", token);

    return RequestContext(true, user->id().get(),
                          user->isAdmin() ? "Admin" : "Student", token);
}

void HttpServer::registerRoutes() {
    server_->set_error_handler([](const httplib::Request& request, httplib::Response& response) {
        if (response.body.empty()) {
            sendError(response, response.status, "Not found: " + request.path);
        }
    });

    server_->set_exception_handler([](const httplib::Request& request, httplib::Response& response,
                                      std::exception_ptr) {
        sendError(response, 500, "Internal server error handling " + request.path);
    });

    // ---- health ----
    server_->Get("/api/healthz", [](const httplib::Request&, httplib::Response& response) {
        json out;
        out["status"] = "ok";
        sendJson(response, 200, out);
    });

    // ---- auth ----
    server_->Post("/api/register", [this](const httplib::Request& request, httplib::Response& response) {
        json body;
        if (!JsonApi::body(request, body)) {
            sendError(response, 400, "Invalid JSON body");
            return;
        }
        const std::string email = stringField(body, "email");
        const std::string password = stringField(body, "password");
        const std::string name = stringField(body, "name", "Student");
        const std::string college = stringField(body, "college", "Unspecified");
        const std::string branch = stringField(body, "branch", "Unspecified");
        const bool asAdmin = intField(body, "role", 0) == 1;

        if (email.empty() || password.empty() || name.empty()) {
            sendError(response, 400, "email, password and name are required");
            return;
        }

        auth::AuthResult result = authService_->registerUser(
            email, password, name,
            asAdmin ? core::UserRole::Admin : core::UserRole::Student, college, branch);
        if (!result.success) {
            sendError(response, 409, result.message);
            return;
        }

        const std::string token = SessionStore::instance().createToken(result.user);
        json out;
        out["token"] = token;
        out["user"] = JsonApi::userToJson(result.user);
        sendJson(response, 201, out);
    });

    server_->Post("/api/login", [this](const httplib::Request& request, httplib::Response& response) {
        json body;
        if (!JsonApi::body(request, body)) {
            sendError(response, 400, "Invalid JSON body");
            return;
        }
        const std::string email = stringField(body, "email");
        const std::string password = stringField(body, "password");
        if (email.empty() || password.empty()) {
            sendError(response, 400, "email and password are required");
            return;
        }

        auth::AuthResult result = authService_->login(email, password);
        if (!result.success) {
            sendError(response, 401, result.message);
            return;
        }

        const std::string token = SessionStore::instance().createToken(result.user);
        json out;
        out["token"] = token;
        out["user"] = JsonApi::userToJson(result.user);
        sendJson(response, 200, out);
    });

    server_->Post("/api/logout", [this](const httplib::Request& request, httplib::Response& response) {
        SessionStore::instance().revokeToken(tokenFromRequest(request));
        json out;
        out["status"] = "signed out";
        sendJson(response, 200, out);
    });

    // ---- profile ----
    server_->Get("/api/me", [this](const httplib::Request& request, httplib::Response& response) {
        const RequestContext context = authenticate(request);
        if (!requireAuth(context, response)) return;

        sendJson(response, 200, JsonApi::userToJson(userRepo_->findById(core::UserId(context.userId()))));
    });

    // ---- catalogue ----
    server_->Get("/api/courses", [this](const httplib::Request&, httplib::Response& response) {
        json out = json::array();
        const std::vector<std::shared_ptr<core::Course>> courses = courseService_->courses();
        for (std::size_t i = 0; i < courses.size(); ++i) {
            json item;
            item["id"] = courses[i]->id();
            item["name"] = courses[i]->name();
            item["description"] = courses[i]->description();
            out.push_back(item);
        }
        sendJson(response, 200, out);
    });

    server_->Get(R"(/api/courses/([^/]+)/subjects)", [this](const httplib::Request& request,
                                                             httplib::Response& response) {
        const std::string courseId = request.matches[1];
        json out = json::array();
        const std::vector<std::shared_ptr<core::Subject>> subjects =
            courseService_->subjectsForCourse(courseId);
        for (std::size_t i = 0; i < subjects.size(); ++i) {
            json item;
            item["id"] = subjects[i]->id();
            item["name"] = subjects[i]->name();
            // Whether this subject has any questions is what the UI wants to
            // show as selectable, so report it rather than letting it fail later.
            item["questionCount"] = questionRepo_->findBySubject(subjects[i]->id()).size();
            out.push_back(item);
        }
        sendJson(response, 200, out);
    });

    // ---- interview ----
    server_->Post("/api/interviews", [this](const httplib::Request& request,
                                            httplib::Response& response) {
        const RequestContext context = authenticate(request);
        if (!requireAuth(context, response)) return;

        json body;
        if (!JsonApi::body(request, body)) {
            sendError(response, 400, "Invalid JSON body");
            return;
        }

        const std::string subject = stringField(body, "subject");
        const std::string modeText = stringField(body, "mode", "Practice");
        core::InterviewSession::Mode mode = core::InterviewSession::Mode::Practice;
        if (modeText == "Mock") mode = core::InterviewSession::Mode::Mock;
        else if (modeText == "Daily") mode = core::InterviewSession::Mode::Daily;

        if (subject.empty()) {
            sendError(response, 400, "subject is required");
            return;
        }

        const core::UserPtr user = userRepo_->findById(core::UserId(context.userId()));
        if (!user) {
            sendError(response, 401, "Account no longer exists");
            return;
        }

        std::unique_ptr<interview::InterviewService> service;
        if (mode == core::InterviewSession::Mode::Daily) {
            service = interview::SessionFactory::createDaily(
                questionRepo_, attemptRepo_, std::make_shared<core::RuleBasedFeedback>());
        } else if (mode == core::InterviewSession::Mode::Mock) {
            service = interview::SessionFactory::createMock(
                questionRepo_, attemptRepo_, std::make_shared<core::RuleBasedFeedback>());
        } else {
            service = interview::SessionFactory::createPractice(
                questionRepo_, attemptRepo_, std::make_shared<core::RuleBasedFeedback>());
        }

        const std::shared_ptr<core::InterviewSession> session =
            service->startSession(user, mode, subject, "");
        if (!session) {
            sendError(response, 404, "No questions available for subject " + subject);
            return;
        }

        const std::string id = SessionStore::instance().createSession(session, user);
        sendJson(response, 201, JsonApi::sessionToJson(*session, session->currentQuestion()));
    });

    server_->Get(R"(/api/interviews/([^/]+))", [this](const httplib::Request& request,
                                                      httplib::Response& response) {
        const RequestContext context = authenticate(request);
        if (!requireAuth(context, response)) return;

        const std::string id = request.matches[1];
        const std::shared_ptr<core::InterviewSession> session =
            SessionStore::instance().session(id);
        if (!session) {
            sendError(response, 404, "Interview not found");
            return;
        }
        if (SessionStore::instance().sessionOwner(id)->id().get() != context.userId()) {
            sendError(response, 403, "Not your interview");
            return;
        }
        sendJson(response, 200, JsonApi::sessionToJson(*session, session->currentQuestion()));
    });

    server_->Post(R"(/api/interviews/([^/]+)/answers)", [this](const httplib::Request& request,
                                                               httplib::Response& response) {
        const RequestContext context = authenticate(request);
        if (!requireAuth(context, response)) return;

        json body;
        if (!JsonApi::body(request, body)) {
            sendError(response, 400, "Invalid JSON body");
            return;
        }

        const std::string id = request.matches[1];
        const std::string answer = stringField(body, "answer");
        const bool skipped = intField(body, "skip", 0) == 1;

        const std::shared_ptr<core::InterviewSession> session =
            SessionStore::instance().session(id);
        if (!session) {
            sendError(response, 404, "Interview not found");
            return;
        }
        if (SessionStore::instance().sessionOwner(id)->id().get() != context.userId()) {
            sendError(response, 403, "Not your interview");
            return;
        }
        if (session->isFinished()) {
            sendError(response, 409, "Interview already finished");
            return;
        }
        if (answer.empty() && !skipped) {
            sendError(response, 400, "answer is required (or send skip=1)");
            return;
        }

        // The service owns the transition, including when the last answer
        // finishes the session.
        std::unique_ptr<interview::InterviewService> service =
            interview::SessionFactory::createPractice(
                questionRepo_, attemptRepo_, std::make_shared<core::RuleBasedFeedback>());
        const core::QuestionPtr question = session->currentQuestion();
        const int seconds = question ? core::timeLimitMinutes(question->difficulty()) * 60 : 120;

        if (skipped) {
            service->skipQuestion(session, seconds);
        } else {
            service->submitAnswer(session, answer, seconds);
        }

        if (session->isFinished()) {
            SessionStore::instance().removeSession(id);
        }
        sendJson(response, 200, JsonApi::sessionToJson(*session, session->currentQuestion()));
    });

    server_->Post(R"(/api/interviews/([^/]+)/finish)", [this](const httplib::Request& request,
                                                              httplib::Response& response) {
        const RequestContext context = authenticate(request);
        if (!requireAuth(context, response)) return;

        const std::string id = request.matches[1];
        const std::shared_ptr<core::InterviewSession> session =
            SessionStore::instance().session(id);
        if (!session) {
            sendError(response, 404, "Interview not found");
            return;
        }
        const core::UserPtr owner = SessionStore::instance().sessionOwner(id);
        if (owner->id().get() != context.userId()) {
            sendError(response, 403, "Not your interview");
            return;
        }

        std::unique_ptr<interview::InterviewService> service =
            interview::SessionFactory::createPractice(
                questionRepo_, attemptRepo_, std::make_shared<core::RuleBasedFeedback>());
        const core::FeedbackReport report = service->finishSession(session);
        progressService_->finalizeSession(owner, report);

        SessionStore::instance().removeSession(id);

        json out;
        out["report"] = JsonApi::reportToJson(report);
        out["streak"] = 0;
        if (auto student = std::dynamic_pointer_cast<core::Student>(
                userRepo_->findById(core::UserId(context.userId())))) {
            out["streak"] = student->streak();
        }
        sendJson(response, 200, out);
    });

    // ---- daily challenge ----
    server_->Get("/api/daily-challenge", [this](const httplib::Request& request,
                                                httplib::Response& response) {
        const RequestContext context = authenticate(request);
        if (!requireAuth(context, response)) return;

        std::string courseId = request.get_param_value("courseId");
        if (courseId.empty() && !courseService_->courses().empty()) {
            courseId = courseService_->courses().front()->id();
        }
        if (courseId.empty()) {
            sendError(response, 404, "No courses available");
            return;
        }

        core::Optional<core::DailyChallenge> challenge =
            progressService_->ensureDailyChallenge(courseId);
        if (!challenge.has_value()) {
            sendError(response, 404, "No daily challenge available today");
            return;
        }

        json out;
        out["date"] = progressService_->todayKey();
        out["courseId"] = challenge.value().courseId;
        out["bonusPoints"] = challenge.value().bonusPoints;
        const core::QuestionPtr question =
            questionRepo_->findById(challenge.value().questionId);
        out["question"] = question ? JsonApi::questionToJson(question) : json(nullptr);
        sendJson(response, 200, out);
    });

    // ---- code execution (Run / Submit for coding problems) ----
    server_->Post(R"(/api/problems/([^/]+)/run)", [this](const httplib::Request& request,
                                                        httplib::Response& response) {
        const RequestContext context = authenticate(request);
        if (!requireAuth(context, response)) return;

        json body;
        if (!JsonApi::body(request, body)) {
            sendError(response, 400, "Invalid JSON body");
            return;
        }
        const std::string code = stringField(body, "code");
        const std::string language = stringField(body, "language", "cpp");
        if (code.empty()) {
            sendError(response, 400, "code is required");
            return;
        }

        try {
            const problem::ProblemService::CodeExecutionResult result =
                problemService_->runCode(core::QuestionId(request.matches[1]),
                                        core::UserId(context.userId()), code, language);
            json out;
            out["verdict"] = result.verdict;
            out["allPassed"] = result.allPassed;
            out["passedCount"] = result.passedCount;
            out["totalCount"] = result.totalCount;
            out["totalTimeMs"] = static_cast<long long>(result.totalTimeMs);
            out["peakMemoryKb"] = static_cast<long long>(result.peakMemoryKb);
            out["compileError"] = result.compileError;

            json cases = json::array();
            for (std::size_t i = 0; i < result.results.size(); ++i) {
                const execution::TestCaseResult& testCase = result.results[i];
                json item;
                item["index"] = static_cast<int>(i) + 1;
                item["passed"] = testCase.passed;
                item["output"] = testCase.output;
                item["expected"] = testCase.expected;
                item["error"] = testCase.error;
                item["errorType"] = testCase.errorType;
                item["exitCode"] = testCase.exitCode;
                item["timeMs"] = static_cast<long long>(testCase.timeMs);
                item["memoryKb"] = static_cast<long long>(testCase.memoryKb);
                cases.push_back(item);
            }
            out["results"] = cases;
            sendJson(response, 200, out);
        } catch (const std::exception& e) {
            sendError(response, 400, e.what());
        }
    });

    // ---- problems catalogue (LeetCode-style browsing) ----
    server_->Get("/api/problems", [this](const httplib::Request& request,
                                         httplib::Response& response) {
        const RequestContext context = authenticate(request);

        problem::ProblemFilter filter;
        filter.subject = paramOr(request, "subject", "");
        filter.difficulty = paramOr(request, "difficulty", "");
        filter.topic = paramOr(request, "topic", "");
        filter.type = paramOr(request, "type", "");
        filter.status = paramOr(request, "status", "");
        filter.search = paramOr(request, "q", "");
        filter.bookmarkedOnly = paramOr(request, "bookmarked", "") == "1";
        // limit=0 means "everything"; the default keeps responses small.
        filter.limit = parseLimit(request, 200);

        const std::string sort = paramOr(request, "sort", "difficulty");
        if (sort == "title") filter.sort = problem::ProblemFilter::Sort::Title;
        else if (sort == "newest") filter.sort = problem::ProblemFilter::Sort::Newest;
        else filter.sort = problem::ProblemFilter::Sort::Difficulty;
        filter.order = paramOr(request, "order", "asc") == "asc"
                           ? problem::ProblemFilter::Order::Ascending
                           : problem::ProblemFilter::Order::Descending;

        // The caller's id drives the per-user status; anonymous callers just
        // see the catalogue without progress.
        const core::UserId userId(context.authenticated() ? core::UserId(context.userId())
                                                         : core::UserId(""));
        const std::vector<problem::ProblemSummary> summaries =
            problemService_->listProblems(filter, userId);

        json out = json::array();
        for (std::size_t i = 0; i < summaries.size(); ++i) {
            json item = problemToJson(summaries[i].question);
            item["solved"] = summaries[i].solved;
            item["attempted"] = summaries[i].attempted;
            item["bookmarked"] = summaries[i].bookmarked;
            item["attempts"] = summaries[i].attempts;
            out.push_back(item);
        }
        sendJson(response, 200, out);
    });

    server_->Get(R"(/api/problems/([^/]+))", [this](const httplib::Request& request,
                                                    httplib::Response& response) {
        const RequestContext context = authenticate(request);
        const core::QuestionId id(request.matches[1]);

        const core::UserId userId(context.authenticated() ? core::UserId(context.userId())
                                                         : core::UserId(""));
        const problem::ProblemSummary summary = problemService_->getProblemSummary(id, userId);
        if (!summary.question) {
            sendError(response, 404, "Problem not found");
            return;
        }

        json out = problemToJson(summary.question);
        out["solved"] = summary.solved;
        out["attempted"] = summary.attempted;
        out["bookmarked"] = summary.bookmarked;
        out["attempts"] = summary.attempts;
        out["needsAuth"] = !context.authenticated();
        sendJson(response, 200, out);
    });

    server_->Get(R"(/api/problems/([^/]+)/submissions)", [this](const httplib::Request& request,
                                                                httplib::Response& response) {
        const RequestContext context = authenticate(request);
        if (!requireAuth(context, response)) return;

        const std::vector<problem::Submission> attempts =
            problemService_->submissions(core::UserId(context.userId()),
                                        core::QuestionId(request.matches[1]), 25);
        json out = json::array();
        for (std::size_t i = 0; i < attempts.size(); ++i) {
            out.push_back(submissionToJson(attempts[i]));
        }
        sendJson(response, 200, out);
    });

    server_->Post(R"(/api/problems/([^/]+)/solve)", [this](const httplib::Request& request,
                                                           httplib::Response& response) {
        const RequestContext context = authenticate(request);
        if (!requireAuth(context, response)) return;

        json body;
        if (!JsonApi::body(request, body)) {
            sendError(response, 400, "Invalid JSON body");
            return;
        }
        const std::string answer = stringField(body, "answer");
        if (answer.empty()) {
            sendError(response, 400, "answer is required");
            return;
        }

        try {
            const problem::Submission submission =
                problemService_->solveProblem(core::QuestionId(request.matches[1]),
                                              core::UserId(context.userId()), answer,
                                              stringField(body, "feedback"));
            json out = submissionToJson(submission);
            // Include the refreshed totals so the caller can update the UI
            // without a second round trip.
            const problem::UserStats stats =
                problemService_->stats(core::UserId(context.userId()));
            out["stats"] = statsToJson(stats);
            sendJson(response, 200, out);
        } catch (const std::exception& e) {
            sendError(response, 400, e.what());
        }
    });

    server_->Post(R"(/api/problems/([^/]+)/bookmark)", [this](const httplib::Request& request,
                                                              httplib::Response& response) {
        const RequestContext context = authenticate(request);
        if (!requireAuth(context, response)) return;

        const bool bookmarked = problemService_->toggleBookmark(
            core::QuestionId(request.matches[1]), core::UserId(context.userId()));
        json out;
        out["bookmarked"] = bookmarked;
        sendJson(response, 200, out);
    });

    // ---- submissions history ----
    server_->Get("/api/submissions", [this](const httplib::Request& request,
                                            httplib::Response& response) {
        const RequestContext context = authenticate(request);
        if (!requireAuth(context, response)) return;

        int limit = 25;
        const std::string raw = paramOr(request, "limit", "25");
        if (!raw.empty()) limit = std::atoi(raw.c_str());
        if (limit <= 0 || limit > 200) limit = 25;

        const std::vector<problem::Submission> attempts =
            problemService_->submissions(core::UserId(context.userId()),
                                        core::QuestionId(""), limit);
        json out = json::array();
        for (std::size_t i = 0; i < attempts.size(); ++i) {
            json item = submissionToJson(attempts[i]);
            const core::QuestionPtr question = problemService_->getProblem(attempts[i].questionId);
            item["subject"] = question ? question->subject() : "";
            item["topic"] = question ? question->topic() : "";
            item["title"] = question ? question->text() : "";
            out.push_back(item);
        }
        sendJson(response, 200, out);
    });

    // ---- personal statistics ----
    server_->Get("/api/stats/me", [this](const httplib::Request& request,
                                         httplib::Response& response) {
        const RequestContext context = authenticate(request);
        if (!requireAuth(context, response)) return;

        const problem::UserStats stats =
            problemService_->stats(core::UserId(context.userId()));

        // The streak calendar needs the raw activity dates, so include them.
        json out = statsToJson(stats);
        const std::vector<core::Attempt> history =
            attemptRepo_->findByUser(core::UserId(context.userId()));
        std::set<std::string> days;
        for (std::size_t i = 0; i < history.size(); ++i) {
            days.insert(history[i].timestampAsDateKey());
        }
        json dates = json::array();
        for (std::set<std::string>::const_iterator it = days.begin(); it != days.end(); ++it) {
            dates.push_back(*it);
        }
        out["activeDates"] = dates;
        out["lastActivityDate"] = userRepo_->findById(core::UserId(context.userId()))
                                      ? std::dynamic_pointer_cast<core::Student>(
                                            userRepo_->findById(core::UserId(context.userId())))
                                            ->lastActivityDate()
                                      : "";
        sendJson(response, 200, out);
    });

    // ---- skill profile: hard and soft skill development ----
    auto skillRowToJson = [](const skills::SkillRow& row) -> json {
        json item;
        item["tag"] = row.tag;
        item["kind"] = row.kind;
        item["available"] = row.available;
        item["solved"] = row.solved;
        item["attempted"] = row.attempted;
        item["mastery"] = row.mastery();
        return item;
    };

    server_->Get("/api/skills", [this, skillRowToJson](const httplib::Request& request,
                                                        httplib::Response& response) {
        const RequestContext context = authenticate(request);
        if (!requireAuth(context, response)) return;

        const skills::SkillProfile profile =
            skillsService_->profile(core::UserId(context.userId()));

        json out;
        out["hardMastery"] = profile.hardMastery;
        out["softMastery"] = profile.softMastery;
        json hard = json::array();
        for (std::size_t i = 0; i < profile.hard.size(); ++i) {
            hard.push_back(skillRowToJson(profile.hard[i]));
        }
        json soft = json::array();
        for (std::size_t i = 0; i < profile.soft.size(); ++i) {
            soft.push_back(skillRowToJson(profile.soft[i]));
        }
        json gaps = json::array();
        for (std::size_t i = 0; i < profile.gaps.size() && i < 10; ++i) {
            gaps.push_back(skillRowToJson(profile.gaps[i]));
        }
        out["hard"] = hard;
        out["soft"] = soft;
        out["gaps"] = gaps;
        sendJson(response, 200, out);
    });

    server_->Get("/api/skills/practice", [this](const httplib::Request& request,
                                                 httplib::Response& response) {
        const RequestContext context = authenticate(request);
        if (!requireAuth(context, response)) return;

        const std::string tag = paramOr(request, "tag", "");
        if (tag.empty()) {
            sendError(response, 400, "tag is required");
            return;
        }

        const std::vector<core::QuestionPtr> questions =
            skillsService_->practiceFor(tag, core::UserId(context.userId()));
        json out = json::array();
        for (std::size_t i = 0; i < questions.size(); ++i) {
            out.push_back(problemToJson(questions[i]));
        }
        sendJson(response, 200, out);
    });

    // ---- filter metadata ----
    server_->Get("/api/catalog", [this](const httplib::Request&, httplib::Response& response) {
        json out;
        const std::vector<std::string> subjects = problemService_->subjectsWithQuestions();
        json subjectArray = json::array();
        for (std::size_t i = 0; i < subjects.size(); ++i) {
            json item;
            item["id"] = subjects[i];
            item["name"] = subjects[i];
            const std::vector<core::QuestionPtr> questions =
                questionRepo_->findBySubject(subjects[i]);
            item["total"] = questions.size();
            int easy = 0;
            int medium = 0;
            int hard = 0;
            for (std::size_t q = 0; q < questions.size(); ++q) {
                switch (questions[q]->difficulty()) {
                    case core::Difficulty::Easy: ++easy; break;
                    case core::Difficulty::Medium: ++medium; break;
                    case core::Difficulty::Hard: ++hard; break;
                }
            }
            item["easy"] = easy;
            item["medium"] = medium;
            item["hard"] = hard;
            subjectArray.push_back(item);
        }
        out["subjects"] = subjectArray;

        json topics = json::array();
        const std::vector<std::string> topicList = problemService_->topics("");
        for (std::size_t i = 0; i < topicList.size(); ++i) {
            json item;
            item["id"] = topicList[i];
            item["name"] = topicList[i];
            topics.push_back(item);
        }
        out["topics"] = topics;
        sendJson(response, 200, out);
    });

    // ---- theory: readable articles for every subject ----
    // The DSA sheet, the core CS subjects and soft skills all live in one
    // data/theory.json, so the section is browsable without a session just
    // like the catalogue.
    server_->Get("/api/theory", [this](const httplib::Request& request,
                                       httplib::Response& response) {
        const std::string subject = paramOr(request, "subject", "");
        json out = json::array();
        const std::vector<theory::TheorySummary> articles = theoryService_->articles(subject);
        for (std::size_t i = 0; i < articles.size(); ++i) {
            const theory::TheorySummary& article = articles[i];
            json item;
            item["id"] = article.id;
            item["subject"] = article.subject;
            item["topic"] = article.topic;
            item["kind"] = article.kind;
            item["category"] = article.category;
            item["name"] = article.name;
            item["tagline"] = article.tagline;
            item["icon"] = article.icon;
            item["readingTime"] = article.readingTime;
            item["summary"] = article.summary;
            item["questionCount"] = article.questionCount;
            out.push_back(item);
        }
        sendJson(response, 200, out);
    });

    server_->Get(R"(/api/theory/([^/]+))", [this](const httplib::Request& request,
                                                  httplib::Response& response) {
        const std::string id = request.matches[1];
        theory::TheoryArticle article;
        if (!theoryService_->find(id, article)) {
            sendJson(response, 404, JsonApi::error("No theory article named " + id, 404));
            return;
        }

        json out;
        out["id"] = article.id;
        out["subject"] = article.subject;
        out["topic"] = article.topic;
        out["kind"] = article.kind;
        out["category"] = article.category;
        out["name"] = article.name;
        out["tagline"] = article.tagline;
        out["icon"] = article.icon;
        out["readingTime"] = article.readingTime;
        out["summary"] = article.summary;
        out["practiceTip"] = article.practiceTip;
        out["questionCount"] = theoryService_->questionCountFor(article);

        json keyPoints = json::array();
        for (std::size_t k = 0; k < article.keyPoints.size(); ++k) {
            keyPoints.push_back(article.keyPoints[k]);
        }
        out["keyPoints"] = keyPoints;

        json sections = json::array();
        for (std::size_t s = 0; s < article.sections.size(); ++s) {
            const theory::TheorySection& section = article.sections[s];
            json item;
            item["heading"] = section.heading;
            item["body"] = json::array();
            for (std::size_t b = 0; b < section.body.size(); ++b) {
                item["body"].push_back(section.body[b]);
            }
            item["bullets"] = json::array();
            for (std::size_t b = 0; b < section.bullets.size(); ++b) {
                item["bullets"].push_back(section.bullets[b]);
            }
            sections.push_back(item);
        }
        out["sections"] = sections;

        json examples = json::array();
        for (std::size_t e = 0; e < article.examples.size(); ++e) {
            json item;
            item["title"] = article.examples[e].title;
            item["body"] = article.examples[e].body;
            examples.push_back(item);
        }
        out["examples"] = examples;

        json mistakes = json::array();
        for (std::size_t m = 0; m < article.mistakes.size(); ++m) {
            mistakes.push_back(article.mistakes[m]);
        }
        out["mistakes"] = mistakes;

        sendJson(response, 200, out);
    });

    // Soft skills is the same store filtered to kind="soft", kept as an alias
    // for the name the section has always had.
    server_->Get("/api/soft-skills", [this](const httplib::Request&, httplib::Response& response) {
        json out = json::array();
        const std::vector<theory::TheorySummary> articles = theoryService_->articles();
        for (std::size_t i = 0; i < articles.size(); ++i) {
            const theory::TheorySummary& article = articles[i];
            if (article.kind != "soft") continue;
            json item;
            item["id"] = article.id;
            item["name"] = article.name;
            item["tagline"] = article.tagline;
            item["icon"] = article.icon;
            item["readingTime"] = article.readingTime;
            item["summary"] = article.summary;
            item["questionCount"] = article.questionCount;
            out.push_back(item);
        }
        sendJson(response, 200, out);
    });

    // ---- leaderboard ----
    server_->Get("/api/leaderboard", [this](const httplib::Request& request,
                                            httplib::Response& response) {
        progress::LeaderboardQuery query;
        query.scope = paramOr(request, "scope", "global");
        query.window = paramOr(request, "window", "alltime");
        query.ranking = paramOr(request, "ranking", "score");
        query.topN = 25;

        json out = json::array();
        const std::vector<core::ScoreboardEntry> entries =
            progressService_->leaderboard(query);
        for (std::size_t i = 0; i < entries.size(); ++i) {
            const core::ScoreboardEntry& entry = entries[i];
            json item;
            item["rank"] = entry.rank;
            item["name"] = entry.name;
            item["college"] = entry.college;
            item["branch"] = entry.branch;
            item["score"] = entry.score;
            item["solved"] = entry.solved;
            item["streak"] = entry.streak;
            out.push_back(item);
        }
        sendJson(response, 200, out);
    });

    // ---- admin ----
    server_->Get("/api/admin/stats", [this](const httplib::Request& request,
                                            httplib::Response& response) {
        const RequestContext context = authenticate(request);
        if (!requireAdmin(context, response)) return;

        const admin::PlatformStats stats = adminService_->platformStats();
        json out;
        out["totalUsers"] = stats.totalUsers;
        out["students"] = stats.students;
        out["admins"] = stats.admins;
        out["totalQuestions"] = stats.totalQuestions;
        out["totalAttempts"] = stats.totalAttempts;
        out["averageScore"] = stats.averageScore;
        out["activeStreaks"] = stats.activeStreaks;
        sendJson(response, 200, out);
    });

    server_->Get("/api/admin/users", [this](const httplib::Request& request,
                                            httplib::Response& response) {
        const RequestContext context = authenticate(request);
        if (!requireAdmin(context, response)) return;

        json out = json::array();
        const std::vector<admin::UserSummary> summaries = adminService_->userSummaries();
        for (std::size_t i = 0; i < summaries.size(); ++i) {
            json item = JsonApi::userToJson(summaries[i].user);
            item["solved"] = summaries[i].solved;
            item["attempts"] = summaries[i].attempts;
            out.push_back(item);
        }
        sendJson(response, 200, out);
    });

    server_->Get("/api/admin/questions", [this](const httplib::Request& request,
                                                httplib::Response& response) {
        const RequestContext context = authenticate(request);
        if (!requireAdmin(context, response)) return;

        json out = json::array();
        const std::vector<admin::QuestionStats> stats = adminService_->questionStats();
        for (std::size_t i = 0; i < stats.size(); ++i) {
            json item;
            item["subject"] = stats[i].subject;
            item["easy"] = stats[i].easy;
            item["medium"] = stats[i].medium;
            item["hard"] = stats[i].hard;
            item["total"] = stats[i].total;
            out.push_back(item);
        }
        sendJson(response, 200, out);
    });

    server_->Delete(R"(/api/admin/users/([^/]+))", [this](const httplib::Request& request,
                                                           httplib::Response& response) {
        const RequestContext context = authenticate(request);
        if (!requireAdmin(context, response)) return;

        const core::UserId target(request.matches[1]);
        if (target.get() == context.userId()) {
            sendError(response, 409, "You cannot delete your own account");
            return;
        }
        sendJson(response, adminService_->removeUser(target) ? 200 : 404,
                 adminService_->removeUser(target)
                     ? json{{"status", "deleted"}}
                     : JsonApi::error("User not found", 404));
    });

    // ---- static front-end ----
    // Revalidate on every load. The front-end is edited in place, and without
    // this a browser keeps serving a cached copy after the files change, which
    // looks like the app failed to load.
    server_->set_pre_routing_handler([](const httplib::Request& request,
                                        httplib::Response& response) -> httplib::Server::HandlerResponse {
        const std::string& path = request.path;
        // "/" is size 1, so the length check must allow it through.
        const bool isAsset = !path.empty() && path != "/api/healthz"
            && (path == "/app.js" || path == "/style.css" || path == "/"
                || path == "/index.html");
        if (isAsset) {
            response.set_header("Cache-Control", "no-cache, must-revalidate");
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });

    server_->set_mount_point("/", config_.staticDir);
}

bool HttpServer::start() {
    if (listening_) return true;
    listening_ = server_->listen(config_.host, config_.port) == 0;
    return listening_;
}

void HttpServer::stop() {
    if (server_ && listening_) {
        server_->stop();
    }
    listening_ = false;
}

} // namespace web
