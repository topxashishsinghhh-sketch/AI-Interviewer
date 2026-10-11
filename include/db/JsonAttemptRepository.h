// OOP: Repository Pattern (JSON Implementation)
#pragma once
#include "core/IAttemptRepository.h"
#include "core/Attempt.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include "json.hpp"

namespace db {

class JsonAttemptRepository : public core::IAttemptRepository {
public:
    explicit JsonAttemptRepository(const std::string& filePath);
    
    core::Attempt* findById(const core::AttemptId& id) override; // Returns nullptr if not found
    std::vector<core::Attempt> findByUser(const core::UserId& userId) override;
    std::vector<core::Attempt> findByUserAndDateRange(
        const core::UserId& userId,
        std::chrono::system_clock::time_point from,
        std::chrono::system_clock::time_point to) override;
    std::vector<core::Attempt> findByQuestion(const core::QuestionId& questionId) override;
    void save(const core::Attempt& attempt) override;
    void remove(const core::AttemptId& id) override;
    
private:
    std::string filePath_;
    std::vector<core::Attempt> cache_;
    nlohmann::json load();
    void save(const nlohmann::json& data);
    core::Attempt deserialize(const nlohmann::json& j);
    nlohmann::json serialize(const core::Attempt& attempt);
};

} // namespace db