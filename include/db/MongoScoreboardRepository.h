// OOP: Repository Pattern (MongoDB Implementation)
#pragma once
#include "core/IScoreboardRepository.h"
#include "core/User.h"
#include "core/StrongId.h"
#include "core/Optional.h"
#include <vector>
#include <string>
#include <chrono>

namespace db {

class MongoScoreboardRepository : public core::IScoreboardRepository {
public:
    explicit MongoScoreboardRepository(const std::string& dbName = "ai_interviewer");
    
    void upsert(const core::ScoreboardEntry& entry, 
                const std::string& scope,
                const std::string& window,
                const std::string& ranking) override;
    std::vector<core::ScoreboardEntry> findTop(int n,
                const std::string& scope,
                const std::string& window,
                const std::string& ranking) override;
    core::Optional<core::ScoreboardEntry> findUser(const core::UserId& userId,
                const std::string& scope,
                const std::string& window,
                const std::string& ranking) override;
    int getUserRank(const core::UserId& userId,
                const std::string& scope,
                const std::string& window,
                const std::string& ranking) override;
    
private:
    std::string dbName_;
    std::string collectionName(const std::string& scope, const std::string& window, const std::string& ranking) const;
};

} // namespace db