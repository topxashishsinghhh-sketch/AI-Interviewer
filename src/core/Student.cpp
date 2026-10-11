// OOP: Inheritance (Role Specialization)
#include "core/Student.h"
#include <algorithm>
#include <cstdio>
#include <ctime>

namespace core {

Student::Student(UserId id, std::string email, std::string passwordHash,
                 std::string name, std::string college, std::string branch)
    : User(std::move(id), std::move(email), std::move(passwordHash),
           std::move(name), UserRole::Student, std::move(college), std::move(branch)) {}

void Student::incrementStreak() {
    streak_++;
    if (streak_ > longestStreak_) longestStreak_ = streak_;
}

void Student::resetStreak() {
    streak_ = 0;
}

void Student::recordActivity(const std::string& dateKey) {
    if (dateKey.empty()) return;
    if (dateKey == lastActivityDate_) return; // already recorded today

    if (lastActivityDate_.empty()) {
        streak_ = 1;
    } else {
        // Parse both dates and compare
        int y1 = 0, m1 = 0, d1 = 0;
        int y2 = 0, m2 = 0, d2 = 0;
        if (std::sscanf(lastActivityDate_.c_str(), "%d-%d-%d", &y1, &m1, &d1) == 3 &&
            std::sscanf(dateKey.c_str(), "%d-%d-%d", &y2, &m2, &d2) == 3) {
            // Convert to days since epoch approximation using time_t
            std::tm tm1 = {};
            tm1.tm_year = y1 - 1900;
            tm1.tm_mon = m1 - 1;
            tm1.tm_mday = d1;
            std::tm tm2 = {};
            tm2.tm_year = y2 - 1900;
            tm2.tm_mon = m2 - 1;
            tm2.tm_mday = d2;
            std::time_t t1 = std::mktime(&tm1);
            std::time_t t2 = std::mktime(&tm2);
            double diffDays = std::difftime(t2, t1) / (60.0 * 60.0 * 24.0);
            if (diffDays == 1.0) {
                incrementStreak();
            } else if (diffDays > 1.0) {
                streak_ = 1;
            } else {
                // dateKey is in the past relative to last activity: ignore
                return;
            }
        } else {
            streak_ = 1;
        }
    }

    lastActivityDate_ = dateKey;
    if (streak_ > longestStreak_) longestStreak_ = streak_;
}

void Student::addSolved() {
    totalSolved_++;
}

void Student::addAttempt() {
    totalAttempts_++;
}

void Student::addBadge(const std::string& badge) {
    if (std::find(badges_.begin(), badges_.end(), badge) == badges_.end()) {
        badges_.push_back(badge);
    }
}

void Student::bookmarkQuestion(const QuestionId& id) {
    if (std::find(bookmarkedQuestions_.begin(), bookmarkedQuestions_.end(), id) == bookmarkedQuestions_.end()) {
        bookmarkedQuestions_.push_back(id);
    }
}

void Student::unbookmarkQuestion(const QuestionId& id) {
    auto it = std::find(bookmarkedQuestions_.begin(), bookmarkedQuestions_.end(), id);
    if (it != bookmarkedQuestions_.end()) {
        bookmarkedQuestions_.erase(it);
    }
}

bool Student::isBookmarked(const QuestionId& id) const {
    return std::find(bookmarkedQuestions_.begin(), bookmarkedQuestions_.end(), id) != bookmarkedQuestions_.end();
}

Admin::Admin(UserId id, std::string email, std::string passwordHash,
             std::string name, std::string college, std::string branch)
    : User(std::move(id), std::move(email), std::move(passwordHash),
           std::move(name), UserRole::Admin, std::move(college), std::move(branch)) {}

} // namespace core