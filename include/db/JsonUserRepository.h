// OOP: Repository Pattern (JSON Implementation)
#pragma once
#include "core/IUserRepository.h"
#include "core/User.h"
#include "core/Student.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <fstream>
#include "json.hpp"

namespace db {

class JsonUserRepository : public core::IUserRepository {
public:
    explicit JsonUserRepository(const std::string& filePath);
    
    core::UserPtr findById(const core::UserId& id) override; // Returns nullptr if not found
    core::UserPtr findByEmail(const std::string& email) override; // Returns nullptr if not found
    std::vector<core::UserPtr> findAll() override;
    void save(const core::UserPtr& user) override;
    void remove(const core::UserId& id) override;
    bool exists(const std::string& email) override;
    
private:
    std::string filePath_;
    std::unordered_map<std::string, core::UserPtr> cache_;
    nlohmann::json load();
    void save(const nlohmann::json& data);
    core::UserPtr deserialize(const nlohmann::json& j);
    nlohmann::json serialize(const core::UserPtr& user);
};

} // namespace db