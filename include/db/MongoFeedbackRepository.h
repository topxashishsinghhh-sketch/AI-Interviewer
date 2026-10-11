// OOP: Repository Pattern (MongoDB Implementation)
#pragma once
#include "core/IFeedbackRepository.h"
#include "core/FeedbackGenerator.h"
#include "core/StrongId.h"
#include <vector>
#include <chrono>

namespace db {

class MongoFeedbackRepository : public core::IFeedbackRepository {
public:
    explicit MongoFeedbackRepository(const std::string& dbName = "ai_interviewer");
    
    std::vector<core::FeedbackReport> findByUser(const core::UserId& userId) override;
    std::vector<core::FeedbackReport> findByUserAndDateRange(
        const core::UserId& userId,
        std::chrono::system_clock::time_point from,
        std::chrono::system_clock::time_point to) override;
    void save(const core::FeedbackReport& report, 
              const core::UserId& userId, 
              const core::SessionId& sessionId) override;
    
private:
    std::string dbName_;
};

} // namespace db