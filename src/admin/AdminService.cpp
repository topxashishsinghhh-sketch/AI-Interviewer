// OOP: Service Layer (Admin Business Logic)
#include "admin/AdminService.h"
#include "core/Student.h"
#include "core/Difficulty.h"

namespace admin {

AdminService::AdminService(std::shared_ptr<core::IUserRepository> userRepo,
                           std::shared_ptr<core::IQuestionRepository> questionRepo,
                           std::shared_ptr<core::IAttemptRepository> attemptRepo)
    : userRepo_(std::move(userRepo))
    , questionRepo_(std::move(questionRepo))
    , attemptRepo_(std::move(attemptRepo)) {}

PlatformStats AdminService::platformStats() const {
    PlatformStats stats;
    
    auto users = userRepo_->findAll();
    stats.totalUsers = static_cast<int>(users.size());
    
    std::vector<core::Attempt> allAttempts;
    for (size_t i = 0; i < users.size(); ++i) {
        if (users[i]->isAdmin()) {
            stats.admins++;
            continue;
        }
        stats.students++;
        
        std::vector<core::Attempt> userAttempts = attemptRepo_->findByUser(users[i]->id());
        allAttempts.insert(allAttempts.end(), userAttempts.begin(), userAttempts.end());
        
        if (auto student = std::dynamic_pointer_cast<core::Student>(users[i])) {
            if (student->streak() > 0) stats.activeStreaks++;
        }
    }
    
    stats.totalQuestions = static_cast<int>(questionRepo_->findAll().size());
    stats.totalAttempts = static_cast<int>(allAttempts.size());
    
    if (!allAttempts.empty()) {
        double sum = 0.0;
        for (size_t i = 0; i < allAttempts.size(); ++i) {
            sum += allAttempts[i].score;
        }
        stats.averageScore = sum / static_cast<double>(allAttempts.size());
    }
    
    return stats;
}

std::vector<UserSummary> AdminService::userSummaries() const {
    std::vector<UserSummary> result;
    auto users = userRepo_->findAll();
    
    for (size_t i = 0; i < users.size(); ++i) {
        UserSummary summary;
        summary.user = users[i];
        
        std::vector<core::Attempt> attempts = attemptRepo_->findByUser(users[i]->id());
        summary.attempts = static_cast<int>(attempts.size());
        
        if (auto student = std::dynamic_pointer_cast<core::Student>(users[i])) {
            summary.solved = student->totalSolved();
            summary.streak = student->streak();
        }
        
        result.push_back(summary);
    }
    return result;
}

std::vector<QuestionStats> AdminService::questionStats() const {
    std::vector<QuestionStats> result;
    auto questions = questionRepo_->findAll();
    for (size_t i = 0; i < questions.size(); ++i) {
        std::string subject = questions[i]->subject();
        QuestionStats* found = nullptr;
        for (size_t j = 0; j < result.size(); ++j) {
            if (result[j].subject == subject) {
                found = &result[j];
                break;
            }
        }
        if (!found) {
            QuestionStats qs;
            qs.subject = subject;
            result.push_back(qs);
            found = &result.back();
        }
        
        switch (questions[i]->difficulty()) {
            case core::Difficulty::Easy: found->easy++; break;
            case core::Difficulty::Medium: found->medium++; break;
            case core::Difficulty::Hard: found->hard++; break;
        }
        found->total++;
    }
    return result;
}

bool AdminService::removeUser(const core::UserId& id) {
    if (!userRepo_->findById(id)) return false;
    userRepo_->remove(id);
    return true;
}

bool AdminService::resetStreak(const core::UserId& id) {
    auto user = userRepo_->findById(id);
    if (!user) return false;
    auto student = std::dynamic_pointer_cast<core::Student>(user);
    if (!student) return false;
    student->resetStreak();
    userRepo_->save(user);
    return true;
}

bool AdminService::awardBadge(const core::UserId& id, const std::string& badge) {
    auto user = userRepo_->findById(id);
    if (!user) return false;
    auto student = std::dynamic_pointer_cast<core::Student>(user);
    if (!student) return false;
    student->addBadge(badge);
    userRepo_->save(user);
    return true;
}

} // namespace admin
