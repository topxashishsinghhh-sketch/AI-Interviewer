// OOP: Service Layer (problem catalogue, solving, statistics, code execution)
#pragma once
#include "core/IQuestionRepository.h"
#include "core/IAttemptRepository.h"
#include "core/IUserRepository.h"
#include "core/Question.h"
#include "core/User.h"
#include "core/Student.h"
#include "core/FeedbackGenerator.h"
#include "core/StrongId.h"
#include "core/Difficulty.h"
#include "execution/CodeExecutor.h"
#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace problem {

// How the caller wants the catalogue filtered and ordered.
struct ProblemFilter {
    std::string subject;
    std::string difficulty;   // "Easy" | "Medium" | "Hard" | "" for any
    std::string topic;
    std::string search;       // free text over id/topic/text
    std::string type;         // "MCQ" | "Text" | "Coding" | "" for any
    std::string status;       // "solved" | "attempted" | "unsolved" | "" for any
    bool bookmarkedOnly = false;
    // Caps how many rows are returned. 0 means "no limit". The full bank is
    // ~3300 questions and takes well over a second to serialise, so callers get
    // a page unless they explicitly ask for everything.
    int limit = 200;

    enum class Sort { Newest, Difficulty, Title } sort = Sort::Newest;
    enum class Order { Ascending, Descending } order = Order::Ascending;
};

// A problem row as shown in a list: the question plus the caller's own state.
struct ProblemSummary {
    core::QuestionPtr question;
    bool solved = false;
    bool attempted = false;
    bool bookmarked = false;
    int attempts = 0;
};

// A single judged submission.
struct Submission {
    core::AttemptId id;
    core::QuestionId questionId;
    std::string answer;
    bool correct;
    double score;
    std::string feedback;
    std::string difficulty;
    std::string submittedAt;
};

// Aggregated per-user statistics for the dashboard.
struct DifficultyCount {
    int easy = 0;
    int medium = 0;
    int hard = 0;
    int total() const { return easy + medium + hard; }
};

struct UserStats {
    DifficultyCount solved;
    DifficultyCount available;
    int attempts = 0;
    double acceptancePermille = 0;   // solved/attempts * 1000, avoids float drift
    std::vector<std::pair<std::string, DifficultyCount>> bySubject;
    std::vector<std::string> badges;
    int streak = 0;
    int longestStreak = 0;
};

// Result of running code against test cases
struct CodeExecutionResult {
    bool allPassed = false;
    int passedCount = 0;
    int totalCount = 0;
    long long totalTimeMs = 0;
    long long peakMemoryKb = 0;
    std::vector<execution::TestCaseResult> results;
    std::string compileError;
    std::string verdict; // "Accepted", "Wrong Answer", "Compile Error", etc.
};

class ProblemService {
public:
    ProblemService(std::shared_ptr<core::IQuestionRepository> questionRepo,
                   std::shared_ptr<core::IAttemptRepository> attemptRepo,
                   std::shared_ptr<core::IUserRepository> userRepo,
                   std::shared_ptr<core::FeedbackGenerator> feedback);

    // Catalogue + single problem, annotated with the caller's own progress.
    std::vector<ProblemSummary> listProblems(const ProblemFilter& filter,
                                             const core::UserId& userId) const;
    core::QuestionPtr getProblem(const core::QuestionId& id) const;
    ProblemSummary getProblemSummary(const core::QuestionId& id,
                                     const core::UserId& userId) const;

    // Judges one answer, records the attempt, and updates the user's totals,
    // streak and badges. Returns the verdict and the feedback text.
    Submission solveProblem(const core::QuestionId& id, const core::UserId& userId,
                            const std::string& answer,
                            const std::string& feedbackText);

    // Run code against all test cases (for Coding questions)
    struct CodeExecutionResult {
        bool allPassed = false;
        int passedCount = 0;
        int totalCount = 0;
        long long totalTimeMs = 0;
        long long peakMemoryKb = 0;
        std::vector<execution::TestCaseResult> results;
        std::string compileError;
        std::string verdict; // "Accepted", "Wrong Answer", "Compile Error", etc.
    };
    struct CodeExecutionResult runCode(const core::QuestionId& id,
                                       const core::UserId& userId,
                                       const std::string& code,
                                       const std::string& language);

    std::vector<Submission> submissions(const core::UserId& userId,
                                         const core::QuestionId& questionId = core::QuestionId(""),
                                         int limit = 50) const;

    bool toggleBookmark(const core::QuestionId& id, const core::UserId& userId);
    UserStats stats(const core::UserId& userId) const;
    std::vector<std::string> topics(const std::string& subject) const;

    // Distinct subjects that actually have questions, for the filter dropdown.
    std::vector<std::string> subjectsWithQuestions() const;

public:
    // Per-user view of one question, built from the attempt history.
    struct Progress {
        bool solved = false;
        bool attempted = false;
        int attempts = 0;
    };

    Progress progressFor(const core::QuestionId& id,
                         const std::vector<core::Attempt>& history) const;
    void applyProgress(std::vector<ProblemSummary>& summaries,
                       const std::vector<core::Attempt>& history,
                       const core::Student& student) const;
    int attemptsFor(const core::QuestionId& id, const core::UserId& userId) const;
    static std::string isoStamp(const std::chrono::system_clock::time_point& tp);
    static std::string todayKey();

    bool explainMode_ = false;

    std::shared_ptr<core::IQuestionRepository> questionRepo_;
    std::shared_ptr<core::IAttemptRepository> attemptRepo_;
    std::shared_ptr<core::IUserRepository> userRepo_;
    std::shared_ptr<core::FeedbackGenerator> feedback_;
    std::shared_ptr<execution::CodeExecutor> codeExecutor_;
};

} // namespace problem