// OOP: Interface Segregation (Repository Contract)
#pragma once
#include "core/User.h"
#include "core/StrongId.h"
#include "core/Optional.h"
#include <vector>
#include <string>
#include <chrono>

namespace core {

struct ScoreboardEntry {
    UserId userId;
    std::string name;
    std::string college;
    std::string branch;
    double score;
    int solved;
    int streak;
    std::chrono::system_clock::time_point lastActivity;
    int rank;
};

class IScoreboardRepository {
public:
    virtual ~IScoreboardRepository() = default;
    virtual void upsert(const ScoreboardEntry& entry, 
                        const std::string& scope,
                        const std::string& window,
                        const std::string& ranking) = 0;
    virtual std::vector<ScoreboardEntry> findTop(int n,
                        const std::string& scope,
                        const std::string& window,
                        const std::string& ranking) = 0;
    virtual Optional<ScoreboardEntry> findUser(const UserId& userId,
                        const std::string& scope,
                        const std::string& window,
                        const std::string& ranking) = 0;
    virtual int getUserRank(const UserId& userId,
                        const std::string& scope,
                        const std::string& window,
                        const std::string& ranking) = 0;
};

} // namespace core