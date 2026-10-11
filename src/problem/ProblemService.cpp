// OOP: Service Layer (problem catalogue, solving, statistics, code execution)
#include "problem/ProblemService.h"
#include "core/Student.h"
#include "core/Difficulty.h"
#include "core/Attempt.h"
#include "core/StrongId.h"
#include "core/CodingQuestion.h"
#include "execution/CodeExecutor.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <set>

namespace problem {
namespace {

std::string toLower(const std::string& text) {
    std::string out = text;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

bool containsText(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    return toLower(haystack).find(toLower(needle)) != std::string::npos;
}

int& bucketFor(DifficultyCount& counts, core::Difficulty difficulty) {
    switch (difficulty) {
        case core::Difficulty::Easy: return counts.easy;
        case core::Difficulty::Medium: return counts.medium;
        case core::Difficulty::Hard: return counts.hard;
    }
    return counts.easy;
}

int difficultyRank(core::Difficulty difficulty) {
    switch (difficulty) {
        case core::Difficulty::Easy: return 0;
        case core::Difficulty::Medium: return 1;
        case core::Difficulty::Hard: return 2;
    }
    return 0;
}

} // namespace

ProblemService::ProblemService(std::shared_ptr<core::IQuestionRepository> questionRepo,
                               std::shared_ptr<core::IAttemptRepository> attemptRepo,
                               std::shared_ptr<core::IUserRepository> userRepo,
                               std::shared_ptr<core::FeedbackGenerator> feedback)
    : questionRepo_(std::move(questionRepo))
    , attemptRepo_(std::move(attemptRepo))
    , userRepo_(std::move(userRepo))
    , feedback_(std::move(feedback))
    , codeExecutor_(std::make_shared<execution::CodeExecutor>()) {}

ProblemService::Progress ProblemService::progressFor(
    const core::QuestionId& id, const std::vector<core::Attempt>& history) const {
    Progress progress;
    for (std::size_t i = 0; i < history.size(); ++i) {
        if (history[i].questionId != id) continue;
        progress.attempted = true;
        ++progress.attempts;
        if (history[i].correct) progress.solved = true;
    }
    return progress;
}

void ProblemService::applyProgress(std::vector<ProblemSummary>& summaries,
                                   const std::vector<core::Attempt>& history,
                                   const core::Student& student) const {
    for (std::size_t i = 0; i < summaries.size(); ++i) {
        const Progress progress = progressFor(summaries[i].question->id(), history);
        summaries[i].attempts = progress.attempts;
        summaries[i].attempted = progress.attempted;
        summaries[i].solved = progress.solved;
        summaries[i].bookmarked = student.isBookmarked(summaries[i].question->id());
    }
}

std::vector<ProblemSummary> ProblemService::listProblems(
    const ProblemFilter& filter, const core::UserId& userId) const {
    std::vector<core::QuestionPtr> questions = questionRepo_->findAll();

    // Fetch the caller's history once rather than per question.
    std::vector<core::Attempt> history;
    if (!userId.get().empty()) {
        history = attemptRepo_->findByUser(userId);
    }
    core::UserPtr user = userRepo_->findById(userId);
    auto student = std::dynamic_pointer_cast<core::Student>(user);
    if (!student) {
        student = std::make_shared<core::Student>(core::UserId(""), "", "", "", "", "");
    }

    std::vector<ProblemSummary> summaries;
    for (std::size_t i = 0; i < questions.size(); ++i) {
        const core::QuestionPtr& question = questions[i];

        if (!filter.subject.empty() && question->subject() != filter.subject) continue;
        if (!filter.topic.empty() && question->topic() != filter.topic) continue;
        if (!filter.difficulty.empty() &&
            core::toString(question->difficulty()) != filter.difficulty) continue;
        if (!filter.type.empty() && question->type() != filter.type) continue;

        const Progress progress = progressFor(question->id(), history);
        if (filter.status == "solved" && !progress.solved) continue;
        if (filter.status == "attempted" && !progress.attempted) continue;
        if (filter.status == "unsolved" && progress.attempted) continue;

        if (filter.bookmarkedOnly && !student->isBookmarked(question->id())) continue;

        if (!filter.search.empty() &&
            !containsText(question->id().get(), filter.search) &&
            !containsText(question->topic(), filter.search) &&
            !containsText(question->text(), filter.search)) continue;

        ProblemSummary summary;
        summary.question = question;
        summary.solved = progress.solved;
        summary.attempted = progress.attempted;
        summary.attempts = progress.attempts;
        summary.bookmarked = student->isBookmarked(question->id());
        summaries.push_back(summary);
    }

    const bool ascending = filter.order == ProblemFilter::Order::Ascending;
    if (filter.sort == ProblemFilter::Sort::Difficulty) {
        std::stable_sort(summaries.begin(), summaries.end(),
            [ascending](const ProblemSummary& a, const ProblemSummary& b) {
                const int ra = difficultyRank(a.question->difficulty());
                const int rb = difficultyRank(b.question->difficulty());
                return ascending ? ra < rb : ra > rb;
            });
    } else if (filter.sort == ProblemFilter::Sort::Title) {
        std::stable_sort(summaries.begin(), summaries.end(),
            [ascending](const ProblemSummary& a, const ProblemSummary& b) {
                return ascending ? a.question->id().get() < b.question->id().get()
                                 : a.question->id().get() > b.question->id().get();
            });
    }

    // Trim to the requested page size after ordering, so a limit changes which
    // rows appear rather than truncating before the sort is applied.
    if (filter.limit > 0 && static_cast<int>(summaries.size()) > filter.limit) {
        summaries.resize(static_cast<std::size_t>(filter.limit));
    }
    return summaries;
}

core::QuestionPtr ProblemService::getProblem(const core::QuestionId& id) const {
    return questionRepo_->findById(id);
}

ProblemSummary ProblemService::getProblemSummary(
    const core::QuestionId& id, const core::UserId& userId) const {
    ProblemSummary summary;
    summary.question = getProblem(id);
    if (!summary.question) return summary;

    std::vector<core::Attempt> history;
    if (!userId.get().empty()) {
        history = attemptRepo_->findByUser(userId);
    }
    core::UserPtr user = userRepo_->findById(userId);
    auto student = std::dynamic_pointer_cast<core::Student>(user);
    if (!student) {
        student = std::make_shared<core::Student>(core::UserId(""), "", "", "", "", "");
    }

    std::vector<ProblemSummary> single;
    single.push_back(summary);
    applyProgress(single, history, *student);
    return single.front();
}

Submission ProblemService::solveProblem(
    const core::QuestionId& id, const core::UserId& userId,
    const std::string& answer, const std::string& feedbackText) {
    const core::QuestionPtr question = getProblem(id);
    if (!question) {
        throw std::runtime_error("Unknown problem: " + id.get());
    }
    core::UserPtr user = userRepo_->findById(userId);
    if (!user) {
        throw std::runtime_error("Unknown user: " + userId.get());
    }

    if (explainMode_) {
        std::cout << "[OOP: Question.evaluate() - Polymorphism]\n";
    }
    const core::EvaluationResult verdict = question->evaluate(answer);

    core::Attempt attempt(core::AttemptId("att_" + id.get() + "_" +
                                          std::to_string(attemptsFor(id, userId) + 1)),
                          userId, id, answer, verdict.correct, verdict.score,
                          question->difficulty(), 0);
    attempt.bookmarked = false;
    attempt.feedback = feedbackText.empty() ? verdict.feedback : feedbackText;
    attemptRepo_->save(attempt);

    if (auto student = std::dynamic_pointer_cast<core::Student>(user)) {
        student->addAttempt();
        if (verdict.correct) student->addSolved();

        const std::string today = todayKey();
        student->recordActivity(today);

        if (student->totalSolved() >= 10) student->addBadge("Problem Solver");
        if (student->totalSolved() >= 50) student->addBadge("Code Crusher");
        if (student->streak() >= 7) student->addBadge("Week Warrior");
        if (student->streak() >= 30) student->addBadge("Monthly Master");
        userRepo_->save(user);
    }

    Submission submission;
    submission.id = attempt.id;
    submission.questionId = id;
    submission.answer = answer;
    submission.correct = attempt.correct;
    submission.score = attempt.score;
    submission.feedback = attempt.feedback;
    submission.difficulty = core::toString(question->difficulty());
    submission.submittedAt = isoStamp(attempt.timestamp);
    return submission;
}

std::vector<Submission> ProblemService::submissions(
    const core::UserId& userId, const core::QuestionId& questionId, int limit) const {
    std::vector<core::Attempt> history = attemptRepo_->findByUser(userId);

    std::vector<Submission> out;
    for (std::size_t i = 0; i < history.size(); ++i) {
        if (!questionId.get().empty() && history[i].questionId != questionId) continue;

        Submission submission;
        submission.id = history[i].id;
        submission.questionId = history[i].questionId;
        submission.answer = history[i].answer;
        submission.correct = history[i].correct;
        submission.score = history[i].score;
        submission.feedback = history[i].feedback;
        submission.difficulty = core::toString(history[i].difficulty);
        submission.submittedAt = isoStamp(history[i].timestamp);
        out.push_back(submission);
    }

    std::sort(out.begin(), out.end(),
        [](const Submission& a, const Submission& b) {
            return a.submittedAt > b.submittedAt;
        });

    if (limit > 0 && static_cast<int>(out.size()) > limit) {
        out.resize(static_cast<std::size_t>(limit));
    }
    return out;
}

bool ProblemService::toggleBookmark(const core::QuestionId& id, const core::UserId& userId) {
    core::UserPtr user = userRepo_->findById(userId);
    auto student = std::dynamic_pointer_cast<core::Student>(user);
    if (!student) return false;

    if (student->isBookmarked(id)) {
        student->unbookmarkQuestion(id);
    } else {
        student->bookmarkQuestion(id);
    }
    userRepo_->save(user);
    return student->isBookmarked(id);
}

UserStats ProblemService::stats(const core::UserId& userId) const {
    UserStats stats;
    if (userId.get().empty()) return stats;

    const std::vector<core::Attempt> history = attemptRepo_->findByUser(userId);
    const std::vector<core::QuestionPtr> questions = questionRepo_->findAll();

    core::UserPtr user = userRepo_->findById(userId);
    if (auto student = std::dynamic_pointer_cast<core::Student>(user)) {
        stats.badges = student->badges();
        stats.streak = student->streak();
        stats.longestStreak = student->longestStreak();
    }

    std::map<std::string, DifficultyCount> bySubject;
    for (std::size_t i = 0; i < questions.size(); ++i) {
        bucketFor(bySubject[questions[i]->subject()], questions[i]->difficulty())++;
        bucketFor(stats.available, questions[i]->difficulty())++;
    }
    for (std::map<std::string, DifficultyCount>::const_iterator it = bySubject.begin();
         it != bySubject.end(); ++it) {
        stats.bySubject.push_back(std::make_pair(it->first, it->second));
    }

    stats.attempts = static_cast<int>(history.size());

    std::set<core::QuestionId> solvedIds;
    for (std::size_t i = 0; i < history.size(); ++i) {
        if (history[i].correct) solvedIds.insert(history[i].questionId);
    }
    // A question counts once no matter how many attempts it took.
    for (std::set<core::QuestionId>::const_iterator it = solvedIds.begin();
         it != solvedIds.end(); ++it) {
        const core::QuestionPtr question = questionRepo_->findById(*it);
        if (!question) continue;
        bucketFor(stats.solved, question->difficulty())++;
    }

    if (stats.attempts > 0) {
        stats.acceptancePermille =
            1000.0 * static_cast<double>(stats.solved.total()) /
            static_cast<double>(stats.attempts);
    }
    return stats;
}

std::vector<std::string> ProblemService::topics(const std::string& subject) const {
    std::vector<core::QuestionPtr> questions = questionRepo_->findAll();
    std::set<std::string> unique;
    for (std::size_t i = 0; i < questions.size(); ++i) {
        if (!subject.empty() && questions[i]->subject() != subject) continue;
        unique.insert(questions[i]->topic());
    }
    return std::vector<std::string>(unique.begin(), unique.end());
}

std::vector<std::string> ProblemService::subjectsWithQuestions() const {
    std::vector<core::QuestionPtr> questions = questionRepo_->findAll();
    std::set<std::string> unique;
    for (std::size_t i = 0; i < questions.size(); ++i) {
        unique.insert(questions[i]->subject());
    }
    return std::vector<std::string>(unique.begin(), unique.end());
}

std::string ProblemService::isoStamp(const std::chrono::system_clock::time_point& tp) {
    const std::time_t seconds = std::chrono::system_clock::to_time_t(tp);
    const std::tm* tm = std::localtime(&seconds);
    if (!tm) return std::string();
    char buffer[32] = {0};
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d",
                  tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                  tm->tm_hour, tm->tm_min, tm->tm_sec);
    return std::string(buffer);
}

std::string ProblemService::todayKey() {
    const std::time_t seconds = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());
    const std::tm* tm = std::localtime(&seconds);
    if (!tm) return std::string();
    char buffer[16] = {0};
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d",
                  tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
    return std::string(buffer);
}

int ProblemService::attemptsFor(const core::QuestionId& id,
                                const core::UserId& userId) const {
    const std::vector<core::Attempt> history = attemptRepo_->findByUser(userId);
    int count = 0;
    for (std::size_t i = 0; i < history.size(); ++i) {
        if (history[i].questionId == id) ++count;
    }
    return count;
}

ProblemService::CodeExecutionResult ProblemService::runCode(
    const core::QuestionId& id, const core::UserId& userId,
    const std::string& code, const std::string& language) {
    // Running code is a dry run that does not record an attempt, so the caller
    // is only used to reject anonymous requests at the API layer.
    (void)userId;
    CodeExecutionResult result;
    result.verdict = "Internal Error";

    const core::QuestionPtr question = getProblem(id);
    if (!question) {
        result.verdict = "Question Not Found";
        result.compileError = "Question not found";
        return result;
    }

    auto codingQuestion = std::dynamic_pointer_cast<core::CodingQuestion>(question);
    if (!codingQuestion) {
        result.verdict = "Not a Coding Question";
        result.compileError = "This question does not support code execution";
        return result;
    }

    if (!codeExecutor_) {
        result.verdict = "Internal Error";
        result.compileError = "Code execution service unavailable";
        return result;
    }

    std::vector<std::pair<std::string, std::string>> testCases;
    for (std::size_t i = 0; i < codingQuestion->testCases().size(); ++i) {
        testCases.push_back(std::make_pair(codingQuestion->testCases()[i].input,
                                          codingQuestion->testCases()[i].expectedOutput));
    }

    execution::ExecutionResult execResult =
        codeExecutor_->runCode(code, language, testCases, 2000, 256);

    result.allPassed = execResult.allPassed;
    result.passedCount = execResult.passedCount;
    result.totalCount = execResult.totalCount;
    result.totalTimeMs = execResult.totalTimeMs;
    result.peakMemoryKb = execResult.peakMemoryKb;
    result.results = execResult.results;
    result.compileError = execResult.compileError;
    result.verdict = execResult.verdict;
    return result;
}

} // namespace problem
