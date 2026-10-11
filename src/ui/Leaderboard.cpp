// OOP: Template Pattern (Generic Algorithm)
#include "ui/Leaderboard.h"
#include <algorithm>

namespace ui {

template <typename T>
void Leaderboard<T>::addEntry(const Entry& entry) {
    entries_.push_back(entry);
    std::sort(entries_.begin(), entries_.end(), [this](const Entry& a, const Entry& b) {
        return compareEntries(a, b);
    });
    // Update ranks
    for (size_t i = 0; i < entries_.size(); ++i) {
        entries_[i].rank = static_cast<int>(i) + 1;
    }
}

template <typename T>
std::vector<typename Leaderboard<T>::Entry> Leaderboard<T>::top(int n) const {
    std::vector<Entry> result;
    int limit = std::min(n, static_cast<int>(entries_.size()));
    for (int i = 0; i < limit; ++i) {
        result.push_back(entries_[i]);
    }
    return result;
}

template <typename T>
typename Leaderboard<T>::Entry* Leaderboard<T>::findUser(const core::UserId& userId) {
    for (typename std::vector<Entry>::iterator it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->userId == userId) return &(*it);
    }
    return nullptr;
}

template <typename T>
const typename Leaderboard<T>::Entry* Leaderboard<T>::findUser(const core::UserId& userId) const {
    for (typename std::vector<Entry>::const_iterator it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->userId == userId) return &(*it);
    }
    return nullptr;
}

template <typename T>
int Leaderboard<T>::rankOf(const core::UserId& userId) const {
    for (typename std::vector<Entry>::const_iterator it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->userId == userId) return it->rank;
    }
    return -1;
}

template <typename T>
bool Leaderboard<T>::compareEntries(const Entry& a, const Entry& b) const {
    // Primary sort
    if (ranking_ == Ranking::ByScore) {
        if (a.score != b.score) return a.score > b.score;
    } else if (ranking_ == Ranking::BySolved) {
        if (a.solved != b.solved) return a.solved > b.solved;
    } else if (ranking_ == Ranking::ByStreak) {
        if (a.streak != b.streak) return a.streak > b.streak;
    }
    
    // Tie-breakers: score -> solved -> earliest activity
    if (a.score != b.score) return a.score > b.score;
    if (a.solved != b.solved) return a.solved > b.solved;
    return a.lastActivity < b.lastActivity;
}

template <typename T>
bool Leaderboard<T>::inWindow(const Entry& entry) const {
    auto now = std::chrono::system_clock::now();
    switch (window_) {
        case Window::Week: {
            auto weekAgo = now - std::chrono::hours(24 * 7);
            return entry.lastActivity >= weekAgo;
        }
        case Window::Month: {
            auto monthAgo = now - std::chrono::hours(24 * 30);
            return entry.lastActivity >= monthAgo;
        }
        case Window::AllTime:
        default:
            return true;
    }
}

template <typename T>
bool Leaderboard<T>::inScope(const Entry& entry, const std::string& college, const std::string& branch) const {
    switch (scope_) {
        case Scope::ByCollege: return entry.college == college;
        case Scope::ByBranch: return entry.branch == branch;
        case Scope::BySubject: return entry.subject == subjectFilter_;
        case Scope::Global:
        default:
            return true;
    }
}

// Explicit instantiation for common types
template class Leaderboard<double>;
template class Leaderboard<int>;

} // namespace ui