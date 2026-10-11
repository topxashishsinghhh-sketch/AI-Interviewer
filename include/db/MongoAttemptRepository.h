// OOP: Repository Pattern (MongoDB Implementation)
#pragma once
#include "core/IAttemptRepository.h"
#include "core/Attempt.h"
#include <string>
#include <vector>
#include <chrono>

namespace db {

class MongoAttemptRepository : public core::IAttemptRepository {
public:
    explicit MongoAttemptRepository(const std::string& dbName = "ai_interviewer");
    
    core::Attempt* findById(const core::AttemptId& id) override;
    std::vector<core::Attempt> findByUser(const core::UserId& userId) override;
    std::vector<core::Attempt> findByUserAndDateRange(
        const core::UserId& userId,
        std::chrono::system_clock::time_point from,
        std::chrono::system_clock::time_point to) override;
    std::vector<core::Attempt> findByQuestion(const core::QuestionId& questionId) override;
    void save(const core::Attempt& attempt) override;
    void remove(const core::AttemptId& id) override;
    
private:
    std::string dbName_;
};

} // namespace db