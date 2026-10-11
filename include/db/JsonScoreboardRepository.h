// OOP: Repository Pattern (JSON Implementation)
#pragma once
#include "core/IScoreboardRepository.h"
#include <map>
#include <string>
#include <vector>
#include <memory>

namespace db {

class JsonScoreboardRepository : public core::IScoreboardRepository {
public:
    explicit JsonScoreboardRepository(const std::string& filePath);
    
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
    std::string filePath_;
    std::string collectionKey(const std::string& scope, const std::string& window, const std::string& ranking) const;
    void load();
    void save();
    
    struct CollectionData {
        std::vector<core::ScoreboardEntry> entries;
    };
    std::map<std::string, CollectionData> collections_;
};

} // namespace db