// OOP: Template Pattern (Generic Algorithm)
#pragma once
#include "core/Attempt.h"
#include "core/User.h"
#include <vector>
#include <map>
#include <string>
#include <algorithm>

namespace ui {

template <typename T>
class Leaderboard {
public:
    struct Entry {
        core::UserId userId;
        std::string name;
        std::string college;
        std::string branch;
        std::string subject;
        T score;
        int solved;
        int streak;
        std::chrono::system_clock::time_point lastActivity;
        int rank = 0;
    };
    
    enum class Scope { Global, ByCollege, ByBranch, BySubject };
    enum class Window { AllTime, Week, Month };
    enum class Ranking { ByScore, BySolved, ByStreak };
    
    Leaderboard(Scope scope, Window window, Ranking ranking)
        : scope_(scope), window_(window), ranking_(ranking) {}
    
    // Enables Scope::BySubject filtering.
    void setSubjectFilter(std::string subject) { subjectFilter_ = std::move(subject); }
    
    void addEntry(const Entry& entry);
    std::vector<Entry> top(int n = 10) const;
    Entry* findUser(const core::UserId& userId); // Returns nullptr if not found
    const Entry* findUser(const core::UserId& userId) const; // Returns nullptr if not found
    int rankOf(const core::UserId& userId) const;
    
private:
    Scope scope_;
    Window window_;
    Ranking ranking_;
    std::vector<Entry> entries_;
    std::string subjectFilter_;
    
    bool compareEntries(const Entry& a, const Entry& b) const;
    bool inWindow(const Entry& entry) const;
    bool inScope(const Entry& entry, const std::string& college, const std::string& branch) const;
};

} // namespace ui