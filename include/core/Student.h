// OOP: Inheritance (Role Specialization)
#pragma once
#include "core/User.h"
#include <string>
#include <vector>

namespace core {

class Student : public User {
public:
    Student(UserId id, std::string email, std::string passwordHash,
            std::string name, std::string college = "Unspecified",
            std::string branch = "Unspecified");
    
    int streak() const { return streak_; }
    int longestStreak() const { return longestStreak_; }
    int totalSolved() const { return totalSolved_; }
    int totalAttempts() const { return totalAttempts_; }
    const std::string& lastActivityDate() const { return lastActivityDate_; }
    const std::vector<std::string>& badges() const { return badges_; }
    const std::vector<QuestionId>& bookmarkedQuestions() const { return bookmarkedQuestions_; }
    
    // Records activity for a given date key ("YYYY-MM-DD") and updates the streak.
    // Same day = no change, previous day = increment, otherwise reset to 1.
    void recordActivity(const std::string& dateKey);
    void setStreak(int streak) { streak_ = streak; }
    void setLongestStreak(int longest) { longestStreak_ = longest; }
    void setTotalSolved(int solved) { totalSolved_ = solved; }
    void setTotalAttempts(int attempts) { totalAttempts_ = attempts; }
    void setLastActivityDate(const std::string& dateKey) { lastActivityDate_ = dateKey; }
    void incrementStreak();
    void resetStreak();
    void addSolved();
    void addAttempt();
    void addBadge(const std::string& badge);
    void bookmarkQuestion(const QuestionId& id);
    void unbookmarkQuestion(const QuestionId& id);
    bool isBookmarked(const QuestionId& id) const;
    
private:
    int streak_ = 0;
    int longestStreak_ = 0;
    int totalSolved_ = 0;
    int totalAttempts_ = 0;
    std::string lastActivityDate_;
    std::vector<std::string> badges_;
    std::vector<QuestionId> bookmarkedQuestions_;
};

class Admin : public User {
public:
    Admin(UserId id, std::string email, std::string passwordHash,
          std::string name, std::string college = "Unspecified",
          std::string branch = "Unspecified");
};

} // namespace core
