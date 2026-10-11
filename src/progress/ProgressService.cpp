// OOP: Service Layer (Progress and leaderboard business logic)
#include "progress/ProgressService.h"
#include "core/Student.h"
#include "core/StrongId.h"
#include <chrono>
#include <ctime>
#include <cstdio>
#include <sstream>

namespace progress {

ProgressService::ProgressService(std::shared_ptr<core::IUserRepository> userRepo,
                                 std::shared_ptr<core::IQuestionRepository> questionRepo,
                                 std::shared_ptr<core::IScoreboardRepository> scoreboardRepo,
                                 std::shared_ptr<core::IDailyChallengeRepository> dailyChallengeRepo,
                                 std::shared_ptr<course::CourseService> courseService)
    : userRepo_(std::move(userRepo))
    , questionRepo_(std::move(questionRepo))
    , scoreboardRepo_(std::move(scoreboardRepo))
    , dailyChallengeRepo_(std::move(dailyChallengeRepo))
    , courseService_(std::move(courseService)) {}

std::string ProgressService::todayKey() const {
    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm* tm = std::localtime(&t);
    char buf[16] = {0};
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
                  tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
    return std::string(buf);
}

bool ProgressService::isConsecutive(const std::string& fromKey, const std::string& toKey) {
    if (fromKey.empty() || toKey.empty()) return false;
    int y1 = 0, m1 = 0, d1 = 0, y2 = 0, m2 = 0, d2 = 0;
    if (std::sscanf(fromKey.c_str(), "%d-%d-%d", &y1, &m1, &d1) != 3) return false;
    if (std::sscanf(toKey.c_str(), "%d-%d-%d", &y2, &m2, &d2) != 3) return false;
    std::tm a = {};
    a.tm_year = y1 - 1900;
    a.tm_mon = m1 - 1;
    a.tm_mday = d1;
    std::tm b = {};
    b.tm_year = y2 - 1900;
    b.tm_mon = m2 - 1;
    b.tm_mday = d2;
    std::time_t ta = std::mktime(&a);
    std::time_t tb = std::mktime(&b);
    if (ta == static_cast<std::time_t>(-1) || tb == static_cast<std::time_t>(-1)) return false;
    double days = std::difftime(tb, ta) / 86400.0;
    return days == 1.0;
}

int ProgressService::finalizeSession(const core::UserPtr& user, const core::FeedbackReport& report) {
    if (!user) return 0;
    
    auto student = std::dynamic_pointer_cast<core::Student>(user);
    if (student) {
        // 1. daily streak
        student->recordActivity(todayKey());
        
        // 2. cumulative attempt/solved counters
        int correct = 0;
        if (report.overallScore > 0.0) {
            correct = static_cast<int>(report.overallScore * 10.0 + 0.5);
        }
        int attempts = static_cast<int>(report.wrongAnswers.size()) + correct;
        for (int i = 0; i < attempts; ++i) student->addAttempt();
        for (int i = 0; i < correct; ++i) student->addSolved();
        
        // 3. badge milestones
        if (student->totalSolved() >= 10) student->addBadge("Problem Solver");
        if (student->totalSolved() >= 50) student->addBadge("Code Crusher");
        if (student->streak() >= 7) student->addBadge("Week Warrior");
        if (student->streak() >= 30) student->addBadge("Monthly Master");
    }
    
    // 4. persist the updated user
    if (userRepo_) userRepo_->save(user);
    
    // 5. leaderboard entry
    core::ScoreboardEntry entry;
    entry.userId = user->id();
    entry.name = user->name();
    entry.college = user->college();
    entry.branch = user->branch();
    entry.score = report.overallScore;
    entry.streak = 0;
    entry.solved = 0;
    entry.lastActivity = std::chrono::system_clock::now();
    entry.rank = 0;
    if (student) {
        entry.score += static_cast<double>(student->totalSolved());
        entry.solved = student->totalSolved();
        entry.streak = student->streak();
    }
    if (scoreboardRepo_) {
        scoreboardRepo_->upsert(entry, "global", "alltime", "score");
    }
    
    return student ? student->streak() : 0;
}

core::Optional<core::DailyChallenge> ProgressService::ensureDailyChallenge(const std::string& courseId) {
    if (!dailyChallengeRepo_) {
        return core::Optional<core::DailyChallenge>();
    }
    
    std::string key = todayKey();
    core::Optional<core::DailyChallenge> existing = dailyChallengeRepo_->findByCourseAndDate(courseId,
        std::chrono::system_clock::now());
    if (existing.has_value()) {
        return existing;
    }
    
    if (!courseService_ || !questionRepo_ || courseId.empty()) {
        return core::Optional<core::DailyChallenge>();
    }
    
    // Deterministic pick: hash(date + courseId)
    std::string seedSource = key + "|" + courseId;
    std::size_t seed = 0;
    for (std::size_t i = 0; i < seedSource.size(); ++i) {
        seed = seed * 131u + static_cast<unsigned char>(seedSource[i]);
    }
    
    std::vector<core::QuestionPtr> pool;
    std::vector<std::shared_ptr<core::Subject>> subjects = courseService_->subjectsForCourse(courseId);
    for (std::size_t i = 0; i < subjects.size(); ++i) {
        std::vector<core::QuestionPtr> found = questionRepo_->findBySubject(subjects[i]->id());
        pool.insert(pool.end(), found.begin(), found.end());
    }
    if (pool.empty()) {
        return core::Optional<core::DailyChallenge>();
    }
    
    core::DailyChallenge challenge;
    challenge.courseId = courseId;
    challenge.date = std::chrono::system_clock::now();
    challenge.questionId = pool[seed % pool.size()]->id();
    challenge.bonusPoints = 10;
    
    dailyChallengeRepo_->save(challenge);
    return core::Optional<core::DailyChallenge>(challenge);
}

core::Optional<core::DailyChallenge> ProgressService::dailyChallengeForToday(const std::string& courseId) {
    if (!dailyChallengeRepo_ || courseId.empty()) {
        return core::Optional<core::DailyChallenge>();
    }
    return dailyChallengeRepo_->findByCourseAndDate(courseId, std::chrono::system_clock::now());
}

std::vector<core::ScoreboardEntry> ProgressService::leaderboard(const LeaderboardQuery& query) const {
    if (!scoreboardRepo_) return std::vector<core::ScoreboardEntry>();
    return scoreboardRepo_->findTop(query.topN, query.scope, query.window, query.ranking);
}

int ProgressService::userRank(const core::UserId& userId, const LeaderboardQuery& query) const {
    if (!scoreboardRepo_) return -1;
    return scoreboardRepo_->getUserRank(userId, query.scope, query.window, query.ranking);
}

core::Optional<core::ScoreboardEntry> ProgressService::userEntry(const core::UserId& userId,
                                                                const LeaderboardQuery& query) const {
    if (!scoreboardRepo_) return core::Optional<core::ScoreboardEntry>();
    return scoreboardRepo_->findUser(userId, query.scope, query.window, query.ranking);
}

} // namespace progress
