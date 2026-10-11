// OOP: Facade Pattern (UI Entry Point)
#include "ui/ConsoleUI.h"
#include "core/StrongId.h"
#include "core/Difficulty.h"
#include "core/Student.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <limits>
#include <chrono>
#include <ctime>
#include <thread>
#include <algorithm>
#include <cstdlib>
#ifdef USE_MONGO
#include "db/Database.h"
#endif

namespace ui {

namespace {

// The current wall-clock time; the date key itself now lives in ProgressService.
std::chrono::system_clock::time_point now() {
    return std::chrono::system_clock::now();
}

// Reads an environment variable, returning fallback when it is unset/empty.
std::string envOr(const char* name, const std::string& fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') return fallback;
    return std::string(value);
}

} // namespace

ConsoleUI::ConsoleUI(int argc, char* argv[]) {
    // Initialize repositories with MongoDB fallback
    std::string dataDir = "./data";
    db::StorageBackend backend = db::StorageBackend::Json;
    std::string uri = envOr("MONGODB_URI", "mongodb://localhost:27017");
    std::string dbName = envOr("MONGODB_DB", "ai_interviewer");

    // Parse command line for storage backend and connection settings
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--storage" && i + 1 < argc) {
            backend = db::RepositoryFactory::parseBackend(argv[++i]);
        } else if ((arg == "--uri" || arg == "--mongo-uri") && i + 1 < argc) {
            uri = argv[++i];
        } else if ((arg == "--db" || arg == "--mongo-db") && i + 1 < argc) {
            dbName = argv[++i];
        } else if (arg == "--data-dir" && i + 1 < argc) {
            dataDir = argv[++i];
        } else if (arg == "--json") {
            backend = db::StorageBackend::Json;
        }
    }

    // Try MongoDB with fallback to JSON
    if (backend == db::StorageBackend::Mongo) {
#ifdef USE_MONGO
        // The Database singleton must be initialised before any Mongo-backed
        // repository can acquire a client from the pool.
        try {
            db::Database::instance().initialize(uri, dbName);
        } catch (const std::exception& e) {
            std::cerr << "[ConsoleUI] MongoDB unavailable (" << e.what()
                      << "), falling back to JSON\n";
            backend = db::StorageBackend::Json;
        }
#endif
        if (backend == db::StorageBackend::Mongo) {
            questionRepo_ = db::RepositoryFactory::createQuestionRepositoryWithFallback(dataDir + "/questions.json");
            attemptRepo_ = db::RepositoryFactory::createAttemptRepositoryWithFallback(dataDir + "/attempts.json");
            userRepo_ = db::RepositoryFactory::createUserRepositoryWithFallback(dataDir + "/users.json");

            if (!questionRepo_ || !attemptRepo_ || !userRepo_) {
                std::cerr << "Failed to initialize MongoDB repositories, falling back to JSON\n";
                backend = db::StorageBackend::Json;
            }
        }
    }

    if (backend == db::StorageBackend::Json) {
        questionRepo_ = db::RepositoryFactory::createQuestionRepository(backend, dataDir + "/questions.json");
        attemptRepo_ = db::RepositoryFactory::createAttemptRepository(backend, dataDir + "/attempts.json");
        userRepo_ = db::RepositoryFactory::createUserRepository(backend, dataDir + "/users.json");
    }

    // Progress repositories follow the selected backend so data is not split
    // across two stores; the fallback variants are only used when Mongo was
    // requested but could not be initialised.
    if (backend == db::StorageBackend::Mongo) {
#ifdef USE_MONGO
        scoreboardRepo_ = db::RepositoryFactory::createScoreboardRepository(
            backend, dataDir + "/scoreboard.json", dbName);
        dailyChallengeRepo_ = db::RepositoryFactory::createDailyChallengeRepository(
            backend, dataDir + "/daily_challenges.json", dbName);
#else
        scoreboardRepo_ = db::RepositoryFactory::createScoreboardRepositoryWithFallback(dataDir + "/scoreboard.json");
        dailyChallengeRepo_ = db::RepositoryFactory::createDailyChallengeRepositoryWithFallback(dataDir + "/daily_challenges.json");
#endif
    } else {
        scoreboardRepo_ = db::RepositoryFactory::createScoreboardRepositoryWithFallback(dataDir + "/scoreboard.json");
        dailyChallengeRepo_ = db::RepositoryFactory::createDailyChallengeRepositoryWithFallback(dataDir + "/daily_challenges.json");
    }

    courseService_ = std::make_shared<course::CourseService>();
    authService_ = std::make_unique<auth::AuthService>(userRepo_);
    adminService_ = std::make_unique<admin::AdminService>(userRepo_, questionRepo_, attemptRepo_);
    progressService_ = std::make_unique<progress::ProgressService>(
        userRepo_, questionRepo_, scoreboardRepo_, dailyChallengeRepo_, courseService_);
    feedbackGen_ = std::make_shared<core::RuleBasedFeedback>();
    strategy_ = std::make_shared<core::LinearRampStrategy>();
}

int ConsoleUI::run(int argc, char* argv[]) {
    // Check flags
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--explain") {
            explainMode_ = true;
        } else if (arg == "--version") {
            showVersion();
            return 0;
        } else if (arg == "--help") {
            showHelp();
            return 0;
        }
    }

    while (true) {
        if (!currentUser_) {
            std::cout << "\n=== AI Interviewer ===\n";
            std::cout << "1. Register\n2. Login\n3. Exit\nChoice: ";
            int choice = 0;
            if (!(std::cin >> choice)) {
                if (std::cin.eof()) break;
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                continue;
            }
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            if (choice == 1) handleRegister();
            else if (choice == 2) handleLogin();
            else if (choice == 3) break;
        } else {
            // Route admin users to the admin dashboard
            if (currentUser_->isAdmin()) {
                showAdminDashboard(currentUser_);
            } else {
                showDashboard(currentUser_);
            }
        }
    }
    return 0;
}

void ConsoleUI::showHelp() {
    std::cout << "AI Interviewer - C++17 OOP Interview Practice\n";
    std::cout << "Usage: ai_interviewer [--explain] [--version] [--help] [--storage json|mongo]\n";
    std::cout << "  --explain  Show which class handles each step\n";
    std::cout << "  --version  Show version\n";
    std::cout << "  --help     Show this help\n";
    std::cout << "  --storage  Storage backend (json|mongo)\n";
}

void ConsoleUI::showVersion() {
    std::cout << "AI Interviewer v1.0.0\n";
}

bool ConsoleUI::handleRegister() {
    std::string email, password, name, college, branch;
    int roleChoice = 1;

    std::cout << "Email: ";
    std::getline(std::cin, email);
    std::cout << "Password: ";
    std::getline(std::cin, password);
    std::cout << "Name: ";
    std::getline(std::cin, name);
    std::cout << "Role (1=Student, 2=Admin): ";
    if (!(std::cin >> roleChoice)) {
        std::cin.clear();
        roleChoice = 1;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "College: ";
    std::getline(std::cin, college);
    std::cout << "Branch: ";
    std::getline(std::cin, branch);

    core::UserRole role = (roleChoice == 2) ? core::UserRole::Admin : core::UserRole::Student;

    if (explainMode_) {
        std::cout << "[OOP: AuthService.registerUser() - Service Layer]\n";
    }

    auto result = authService_->registerUser(email, password, name, role, college, branch);
    std::cout << result.message << "\n";
    return result.success;
}

bool ConsoleUI::handleLogin() {
    std::string email, password;

    std::cout << "Email: ";
    std::getline(std::cin, email);
    std::cout << "Password: ";
    std::getline(std::cin, password);

    if (explainMode_) {
        std::cout << "[OOP: AuthService.login() - Service Layer]\n";
    }

    auto result = authService_->login(email, password);
    std::cout << result.message << "\n";

    if (result.success) {
        currentUser_ = result.user;
    }
    return result.success;
}

void ConsoleUI::showDashboard(const core::UserPtr& user) {
    while (currentUser_) {
        std::cout << "\n=== Dashboard (" << user->name() << ") ===\n";
        std::cout << "1. Start Interview\n2. Daily Challenge\n3. Profile & Streak\n4. Leaderboard\n5. Logout\nChoice: ";
        int choice = 0;
        if (!(std::cin >> choice)) {
            if (std::cin.eof()) {
                currentUser_.reset();
                return;
            }
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (choice == 1) handleCourseSelection(user);
        else if (choice == 2) {
            auto courses = courseService_->courses();
            if (!courses.empty()) {
                handleDailyChallenge(user, courses[0]->id());
            } else {
                std::cout << "No courses available for daily challenge.\n";
            }
        }
        else if (choice == 3) showProfile(user);
        else if (choice == 4) showLeaderboard(user);
        else if (choice == 5) {
            currentUser_.reset();
            break;
        }
    }
}

void ConsoleUI::promptContinue() {
    std::cout << "\nPress Enter to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void ConsoleUI::showAdminDashboard(const core::UserPtr& user) {
    while (currentUser_) {
        std::cout << "\n=== Admin Dashboard (" << user->name() << ") ===\n";
        std::cout << "1. Platform Statistics\n2. User Management\n3. Question Bank Stats"
                     "\n4. Leaderboard\n5. Logout\nChoice: ";
        int choice = 0;
        if (!(std::cin >> choice)) {
            if (std::cin.eof()) {
                currentUser_.reset();
                return;
            }
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (explainMode_) {
            std::cout << "[OOP: AdminService - Service Layer, read-only admin operations]\n";
        }

        if (choice == 1) showAdminPlatformStats();
        else if (choice == 2) showAdminUsers();
        else if (choice == 3) showAdminQuestionStats();
        else if (choice == 4) showLeaderboard(user);
        else if (choice == 5) {
            currentUser_.reset();
            break;
        }
    }
}

void ConsoleUI::showAdminPlatformStats() {
    if (!adminService_) {
        std::cout << "Admin service unavailable.\n";
        promptContinue();
        return;
    }

    admin::PlatformStats stats = adminService_->platformStats();
    std::cout << "\n=== Platform Statistics ===\n";
    std::cout << "Total Users:      " << stats.totalUsers << "\n";
    std::cout << "  Students:       " << stats.students << "\n";
    std::cout << "  Admins:         " << stats.admins << "\n";
    std::cout << "Questions:        " << stats.totalQuestions << "\n";
    std::cout << "Total Attempts:   " << stats.totalAttempts << "\n";
    std::cout << "Average Score:    " << std::fixed << std::setprecision(2)
              << stats.averageScore << "\n";
    std::cout << "Active Streaks:   " << stats.activeStreaks << "\n";
    promptContinue();
}

void ConsoleUI::showAdminUsers() {
    if (!adminService_) {
        std::cout << "Admin service unavailable.\n";
        promptContinue();
        return;
    }

    std::vector<admin::UserSummary> summaries = adminService_->userSummaries();
    std::cout << "\n=== User Management ===\n";
    std::cout << std::left << std::setw(4) << "#"
              << std::setw(24) << "Email"
              << std::setw(18) << "Name"
              << std::setw(8) << "Role"
              << std::setw(8) << "Solved"
              << std::setw(9) << "Attempts"
              << std::setw(8) << "Streak" << "\n";
    std::cout << std::string(79, '-') << "\n";
    for (size_t i = 0; i < summaries.size(); ++i) {
        const core::UserPtr& u = summaries[i].user;
        std::cout << std::left << std::setw(4) << (i + 1)
                  << std::setw(24) << u->email()
                  << std::setw(18) << u->name()
                  << std::setw(8) << (u->isAdmin() ? "Admin" : "Student")
                  << std::setw(8) << summaries[i].solved
                  << std::setw(9) << summaries[i].attempts
                  << std::setw(8) << summaries[i].streak << "\n";
    }
    if (summaries.empty()) {
        std::cout << "No users registered.\n";
    }

    std::cout << "\nActions: 1. Award badge  2. Reset streak  3. Delete user  0. Back\nChoice: ";
    int pick = 0;
    if (!(std::cin >> pick)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (pick == 0) return;

    std::cout << "Row number: ";
    int row = 0;
    if (!(std::cin >> row)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (row < 1 || row > static_cast<int>(summaries.size())) {
        std::cout << "Invalid row.\n";
        promptContinue();
        return;
    }

    core::UserId target = summaries[static_cast<size_t>(row) - 1].user->id();
    if (pick == 1) {
        std::cout << "Badge name: ";
        std::string badge;
        std::getline(std::cin, badge);
        if (adminService_->awardBadge(target, badge)) {
            std::cout << "Badge awarded.\n";
        } else {
            std::cout << "Could not award badge (admins cannot receive badges).\n";
        }
    } else if (pick == 2) {
        if (adminService_->resetStreak(target)) {
            std::cout << "Streak reset.\n";
        } else {
            std::cout << "Could not reset streak.\n";
        }
    } else if (pick == 3) {
        if (target == currentUser_->id()) {
            std::cout << "You cannot delete your own account.\n";
        } else if (adminService_->removeUser(target)) {
            std::cout << "User deleted.\n";
        } else {
            std::cout << "Could not delete user.\n";
        }
    }
    promptContinue();
}

void ConsoleUI::showAdminQuestionStats() {
    if (!adminService_) {
        std::cout << "Admin service unavailable.\n";
        promptContinue();
        return;
    }

    std::vector<admin::QuestionStats> stats = adminService_->questionStats();
    std::cout << "\n=== Question Bank Stats ===\n";
    std::cout << std::left << std::setw(16) << "Subject"
              << std::setw(8) << "Easy"
              << std::setw(8) << "Medium"
              << std::setw(8) << "Hard"
              << std::setw(8) << "Total" << "\n";
    std::cout << std::string(48, '-') << "\n";
    int easyTotal = 0, mediumTotal = 0, hardTotal = 0, grandTotal = 0;
    for (size_t i = 0; i < stats.size(); ++i) {
        std::cout << std::left << std::setw(16) << stats[i].subject
                  << std::setw(8) << stats[i].easy
                  << std::setw(8) << stats[i].medium
                  << std::setw(8) << stats[i].hard
                  << std::setw(8) << stats[i].total << "\n";
        easyTotal += stats[i].easy;
        mediumTotal += stats[i].medium;
        hardTotal += stats[i].hard;
        grandTotal += stats[i].total;
    }
    std::cout << std::string(48, '-') << "\n";
    std::cout << std::left << std::setw(16) << "TOTAL"
              << std::setw(8) << easyTotal
              << std::setw(8) << mediumTotal
              << std::setw(8) << hardTotal
              << std::setw(8) << grandTotal << "\n";
    promptContinue();
}

void ConsoleUI::handleCourseSelection(const core::UserPtr& user) {
    std::cout << "\n=== Select Course ===\n";
    auto courses = courseService_->courses();
    for (size_t i = 0; i < courses.size(); ++i) {
        std::cout << i + 1 << ". " << courses[i]->name() << " (" << courses[i]->id() << ")\n";
    }
    std::cout << "0. Back\nChoice: ";

    int choice = 0;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        return;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (choice > 0 && choice <= static_cast<int>(courses.size())) {
        handleSubjectSelection(user, courses[choice - 1]->id());
    }
}

void ConsoleUI::handleSubjectSelection(const core::UserPtr& user, const std::string& courseId) {
    auto subjects = courseService_->subjectsForCourse(courseId);
    std::cout << "\n=== Select Subject ===\n";
    for (size_t i = 0; i < subjects.size(); ++i) {
        std::cout << i + 1 << ". " << subjects[i]->name() << " (" << subjects[i]->id() << ")\n";
    }
    std::cout << "0. Back\nChoice: ";

    int choice = 0;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        return;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (choice > 0 && choice <= static_cast<int>(subjects.size())) {
        handleModeSelection(user, courseId, subjects[choice - 1]->id());
    }
}

void ConsoleUI::handleModeSelection(const core::UserPtr& user, const std::string& courseId, const std::string& subjectId) {
    std::cout << "\n=== Select Mode ===\n";
    std::cout << "1. Practice (Linear Ramp)\n2. Mock Interview (Linear Ramp)\n3. Daily Challenge (Adaptive)\n0. Back\nChoice: ";

    int choice = 0;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        return;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    core::InterviewSession::Mode mode;
    if (choice == 1) {
        mode = core::InterviewSession::Mode::Practice;
    } else if (choice == 2) {
        mode = core::InterviewSession::Mode::Mock;
    } else if (choice == 3) {
        handleDailyChallenge(user, courseId);
        return;
    } else {
        return;
    }

    if (explainMode_) {
        std::cout << "[OOP: SessionFactory - Factory Pattern]\n";
        std::cout << "[OOP: InterviewService - Service Layer]\n";
    }

    handleInterview(user, subjectId, "", mode);
}

void ConsoleUI::handleInterview(const core::UserPtr& user, const std::string& subject, const std::string& topic, core::InterviewSession::Mode mode) {
    std::unique_ptr<interview::InterviewService> service;

    if (mode == core::InterviewSession::Mode::Daily) {
        service = interview::SessionFactory::createDaily(questionRepo_, attemptRepo_, feedbackGen_);
    } else if (mode == core::InterviewSession::Mode::Mock) {
        service = interview::SessionFactory::createMock(questionRepo_, attemptRepo_, feedbackGen_);
    } else {
        service = interview::SessionFactory::createPractice(questionRepo_, attemptRepo_, feedbackGen_);
    }

    auto session = service->startSession(user, mode, subject, topic);
    if (!session) {
        std::cout << "No questions available for this subject.\n";
        std::cout << "Press Enter to continue...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }

    std::cout << "\n=== Interview Started ===\n";
    std::cout << "Mode: " << (mode == core::InterviewSession::Mode::Practice ? "Practice" :
                          mode == core::InterviewSession::Mode::Mock ? "Mock" : "Daily") << "\n";
    std::cout << "Total Questions: " << session->questions().size() << "\n\n";

    while (!session->isFinished()) {
        auto question = session->currentQuestion();
        if (!question) break;

        std::cout << "\n--- Question " << (session->currentQuestionIndex() + 1) << "/" << session->questions().size() << " ---\n";
        std::cout << "Difficulty: " << core::toString(question->difficulty()) << "\n";
        std::cout << "Time Limit: " << core::timeLimitMinutes(question->difficulty()) << " min\n";
        std::cout << question->display() << "\n";
        std::cout << "Answer (or 'skip'): ";

        std::string answer;
        if (!std::getline(std::cin, answer)) {
            std::cin.clear();
            break;
        }

        int timeTaken = core::timeLimitMinutes(question->difficulty()) * 60;

        if (answer == "skip") {
            if (explainMode_) {
                std::cout << "[OOP: InterviewSession.skipQuestion() - Entity]\n";
            }
            service->skipQuestion(session, timeTaken);
        } else {
            if (explainMode_) {
                std::cout << "[OOP: InterviewSession.submitAnswer() - Entity]\n";
                std::cout << "[OOP: " << question->type() << "Question.evaluate() - Polymorphism]\n";
            }
            service->submitAnswer(session, answer, timeTaken);
        }
    }

    if (explainMode_) {
        std::cout << "[OOP: InterviewService.finishSession() - Service Layer]\n";
        std::cout << "[OOP: RuleBasedFeedback.generate() - Strategy Pattern]\n";
    }

    auto report = service->finishSession(session);

    std::cout << "\n=== Interview Complete ===\n";
    std::cout << "Overall Score: " << std::fixed << std::setprecision(1) << (report.overallScore * 100) << "%\n";
    std::cout << "Strongest: " << report.strongestTopic << "\n";
    std::cout << "Weakest: " << report.weakestTopic << "\n";
    std::cout << "\nTips:\n";
    for (const auto& tip : report.tips) {
        std::cout << "  - " << tip << "\n";
    }

    // Phase 3: update stats, streak and scoreboard after interview
    finalizeSession(user, report);

    std::cout << "\nPress Enter to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void ConsoleUI::handleDailyChallenge(const core::UserPtr& user, const std::string& courseId) {
    if (!progressService_) {
        std::cout << "Daily challenge unavailable.\n";
        promptContinue();
        return;
    }

    core::Optional<core::DailyChallenge> found = progressService_->ensureDailyChallenge(courseId);
    if (!found.has_value()) {
        std::cout << "No daily challenge available for this course today.\n";
        promptContinue();
        return;
    }
    const core::DailyChallenge challenge = found.value();

    std::cout << "\n=== Daily Challenge ===\n";
    std::cout << "Date: " << progressService_->todayKey() << "\n";
    std::cout << "Bonus Points: " << challenge.bonusPoints << "\n";

    auto question = questionRepo_->findById(challenge.questionId);
    if (!question) {
        std::cout << "Question unavailable.\n";
        promptContinue();
        return;
    }

    std::cout << question->display() << "\n";
    std::cout << "Answer (or 'skip'): ";
    std::string answer;
    if (!std::getline(std::cin, answer)) {
        std::cin.clear();
        return;
    }

    if (explainMode_) {
        std::cout << "[OOP: ProgressService.ensureDailyChallenge() - Service Layer]\n";
        std::cout << "[OOP: SessionFactory.createDaily() - Factory Pattern]\n";
    }

    auto service = interview::SessionFactory::createDaily(questionRepo_, attemptRepo_, feedbackGen_);
    auto session = service->startSession(user, core::InterviewSession::Mode::Daily,
                                          question->subject(), "");

    if (session) {
        int timeTaken = core::timeLimitMinutes(question->difficulty()) * 60;
        if (answer == "skip") {
            service->skipQuestion(session, timeTaken);
        } else {
            service->submitAnswer(session, answer, timeTaken);
        }

        auto report = service->finishSession(session);
        std::cout << "\nOverall Score: " << std::fixed << std::setprecision(1) << (report.overallScore * 100) << "%\n";
        report.overallScore += (challenge.bonusPoints / 100.0);
        std::cout << "With Bonus: " << std::fixed << std::setprecision(1) << (report.overallScore * 100) << "%\n";

        finalizeSession(user, report);
    }

    promptContinue();
}

void ConsoleUI::showProfile(const core::UserPtr& user) {
    std::cout << "\n=== Profile ===\n";
    std::cout << "Name: " << user->name() << "\n";
    std::cout << "Email: " << user->email() << "\n";
    std::cout << "College: " << user->college() << "\n";
    std::cout << "Branch: " << user->branch() << "\n";

    if (auto student = std::dynamic_pointer_cast<core::Student>(user)) {
        std::cout << "Current Streak: " << student->streak() << " days\n";
        std::cout << "Longest Streak: " << student->longestStreak() << " days\n";
        std::cout << "Total Solved: " << student->totalSolved() << "\n";
        std::cout << "Total Attempts: " << student->totalAttempts() << "\n";
        std::cout << "Badges: ";
        if (student->badges().empty()) {
            std::cout << "None\n";
        } else {
            for (const auto& badge : student->badges()) {
                std::cout << badge << " ";
            }
            std::cout << "\n";
        }
    }

    showStreakCalendar(user);

    if (progressService_) {
        progress::LeaderboardQuery query;
        core::Optional<core::ScoreboardEntry> entry = progressService_->userEntry(user->id(), query);
        if (entry.has_value()) {
            std::cout << "Global Rank: " << entry.value().rank << " (score: "
                      << std::fixed << std::setprecision(1) << entry.value().score << ")\n";
        }
    }

    promptContinue();
}

void ConsoleUI::showLeaderboard(const core::UserPtr& user) {
    if (!progressService_) {
        std::cout << "Leaderboard repository unavailable.\n";
        promptContinue();
        return;
    }

    // Filter menus (scope x window x ranking)
    std::string scopes[] = {"global", "college", "branch"};
    std::string windows[] = {"alltime", "week", "month"};
    std::string rankings[] = {"score", "solved", "streak"};
    int scopeIdx = 0;
    int windowIdx = 0;
    int rankingIdx = 0;

    std::cout << "\n=== Leaderboard Filters ===\n";
    std::cout << "Scope: 1. Global  2. My College  3. My Branch  [1]: ";
    std::string line;
    if (std::getline(std::cin, line) && !line.empty()) {
        std::istringstream iss(line);
        int pick = 0;
        if (iss >> pick && pick >= 1 && pick <= 3) scopeIdx = pick - 1;
    }
    std::cout << "Window: 1. All Time  2. Week  3. Month  [1]: ";
    if (std::getline(std::cin, line) && !line.empty()) {
        std::istringstream iss(line);
        int pick = 0;
        if (iss >> pick && pick >= 1 && pick <= 3) windowIdx = pick - 1;
    }
    std::cout << "Ranking: 1. Score  2. Solved  3. Streak  [1]: ";
    if (std::getline(std::cin, line) && !line.empty()) {
        std::istringstream iss(line);
        int pick = 0;
        if (iss >> pick && pick >= 1 && pick <= 3) rankingIdx = pick - 1;
    }

    progress::LeaderboardQuery query;
    query.scope = scopes[scopeIdx];
    query.window = windows[windowIdx];
    query.ranking = rankings[rankingIdx];
    query.topN = 10;

    std::cout << "\n=== Leaderboard ===\n";
    std::cout << "Scope: " << query.scope << " | Window: " << query.window
              << " | Ranking: " << query.ranking << "\n\n";

    std::vector<core::ScoreboardEntry> top = progressService_->leaderboard(query);
    if (top.empty()) {
        std::cout << "No scores recorded yet. Complete an interview to appear here.\n";
    } else {
        std::cout << std::left << std::setw(5) << "Rank"
                  << std::setw(20) << "Name"
                  << std::setw(10) << "Score"
                  << std::setw(8) << "Solved"
                  << std::setw(8) << "Streak" << "\n";
        std::cout << std::string(51, '-') << "\n";
        for (size_t i = 0; i < top.size(); ++i) {
            std::cout << std::left << std::setw(5) << top[i].rank
                      << std::setw(20) << top[i].name
                      << std::setw(10) << std::fixed << std::setprecision(2) << top[i].score
                      << std::setw(8) << top[i].solved
                      << std::setw(8) << top[i].streak << "\n";
        }
    }

    int rank = progressService_->userRank(user->id(), query);
    if (rank > 0) {
        std::cout << "\nYour rank: " << rank << "\n";
    }

    promptContinue();
}

void ConsoleUI::showStreakCalendar(const core::UserPtr& user) {
    std::cout << "\n=== Streak Calendar (Current Month) ===\n";

    std::time_t t = std::chrono::system_clock::to_time_t(now());
    std::tm* tm = std::localtime(&t);
    int year = tm->tm_year + 1900;
    int month = tm->tm_mon + 1;
    int today_day = tm->tm_mday;

    std::cout << "Month: " << year << "-" << std::setw(2) << std::setfill('0') << month << std::setfill(' ') << "\n";
    std::cout << "Su Mo Tu We Th Fr Sa\n";

    std::tm first = {};
    first.tm_year = year - 1900;
    first.tm_mon = month - 1;
    first.tm_mday = 1;
    std::mktime(&first);
    int firstDay = first.tm_wday;

    std::tm monthLen = first;
    monthLen.tm_mon += 1;
    std::mktime(&monthLen);
    int days = monthLen.tm_mday;

    for (int i = 0; i < firstDay; ++i) {
        std::cout << "   ";
    }

    for (int d = 1; d <= days; ++d) {
        if (d == today_day) {
            std::cout << std::setw(2) << std::setfill('0') << d << std::setfill(' ') << " ";
        } else {
            std::cout << std::setw(2) << d << " ";
        }
        if ((firstDay + d) % 7 == 0) {
            std::cout << "\n";
        }
    }
    std::cout << "\n";

    if (auto student = std::dynamic_pointer_cast<core::Student>(user)) {
        std::cout << "Current streak: " << student->streak() << " day(s)\n";
        std::cout << "Longest streak: " << student->longestStreak() << " day(s)\n";
    }
}

void ConsoleUI::finalizeSession(const core::UserPtr& user, const core::FeedbackReport& report) {
    if (!progressService_) return;
    if (explainMode_) {
        std::cout << "[OOP: ProgressService.finalizeSession() - Service Layer]\n";
    }
    int streak = progressService_->finalizeSession(user, report);
    std::cout << "[OOP: ProgressService.finalizeSession() - Streak: " << streak << " day(s)]\n";
    std::cout << "[OOP: ScoreboardRepository.upsert() - Ranked Materials]\n";
}

} // namespace ui
