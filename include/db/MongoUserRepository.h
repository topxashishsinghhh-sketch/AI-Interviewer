// OOP: Repository Pattern (MongoDB Implementation)
#pragma once
#include "core/IUserRepository.h"
#include "core/User.h"
#include "core/Student.h"
#include <string>
#include <vector>
#include <memory>

namespace db {

class MongoUserRepository : public core::IUserRepository {
public:
    explicit MongoUserRepository(const std::string& dbName = "ai_interviewer");
    
    core::UserPtr findById(const core::UserId& id) override;
    core::UserPtr findByEmail(const std::string& email) override;
    std::vector<core::UserPtr> findAll() override;
    void save(const core::UserPtr& user) override;
    void remove(const core::UserId& id) override;
    bool exists(const std::string& email) override;
    
private:
    std::string dbName_;
};

} // namespace db