// OOP: Interface Segregation (Repository Contract)
#pragma once
#include "core/FeedbackGenerator.h"
#include "core/StrongId.h"
#include <vector>
#include <chrono>

namespace core {

class IFeedbackRepository {
public:
    virtual ~IFeedbackRepository() = default;
    virtual std::vector<FeedbackReport> findByUser(const UserId& userId) = 0;
    virtual std::vector<FeedbackReport> findByUserAndDateRange(
        const UserId& userId,
        std::chrono::system_clock::time_point from,
        std::chrono::system_clock::time_point to) = 0;
    virtual void save(const FeedbackReport& report, const UserId& userId, const SessionId& sessionId) = 0;
};

} // namespace core