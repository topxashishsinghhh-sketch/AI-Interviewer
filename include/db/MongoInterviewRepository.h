// OOP: Repository Pattern (MongoDB Implementation)
#pragma once
#include "core/IInterviewRepository.h"
#include "core/InterviewSession.h"
#include "core/StrongId.h"
#include <string>
#include <vector>
#include <memory>

namespace db {

class MongoInterviewRepository : public core::IInterviewRepository {
public:
    explicit MongoInterviewRepository(const std::string& dbName = "ai_interviewer");
    
    std::shared_ptr<core::InterviewSession> findById(const core::SessionId& id) override;
    std::vector<std::shared_ptr<core::InterviewSession>> findByUser(const core::UserId& userId) override;
    void save(const std::shared_ptr<core::InterviewSession>& session) override;
    void update(const std::shared_ptr<core::InterviewSession>& session) override;
    
private:
    std::string dbName_;
};

} // namespace db