// OOP: Interface Segregation (Repository Contract)
#pragma once
#include "core/InterviewSession.h"
#include "core/StrongId.h"
#include <vector>

namespace core {

class IInterviewRepository {
public:
    virtual ~IInterviewRepository() = default;
    virtual std::shared_ptr<InterviewSession> findById(const SessionId& id) = 0;
    virtual std::vector<std::shared_ptr<InterviewSession>> findByUser(const UserId& userId) = 0;
    virtual void save(const std::shared_ptr<InterviewSession>& session) = 0;
    virtual void update(const std::shared_ptr<InterviewSession>& session) = 0;
};

} // namespace core