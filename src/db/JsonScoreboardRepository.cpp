// OOP: Repository Pattern (JSON Implementation)
#include "db/JsonScoreboardRepository.h"
#include "core/StrongId.h"
#include "db/JsonTime.h"
#include "json.hpp"
#include <fstream>
#include <algorithm>

using json = nlohmann::json;

namespace db {

JsonScoreboardRepository::JsonScoreboardRepository(const std::string& filePath)
    : filePath_(filePath) {
    load();
}

void JsonScoreboardRepository::load() {
    std::ifstream file(filePath_);
    if (!file.is_open()) return;

    json data;
    file >> data;

    for (json::iterator it = data.begin(); it != data.end(); ++it) {
        std::string key = it.key();
        const json& arr = it.value();
        CollectionData coll;
        for (json::const_iterator jt = arr.begin(); jt != arr.end(); ++jt) {
            const json& item = *jt;
            core::ScoreboardEntry entry;
            entry.userId = core::UserId(item["userId"].get<std::string>());
            entry.name = item["name"].get<std::string>();
            entry.college = item["college"].get<std::string>();
            entry.branch = item["branch"].get<std::string>();
            entry.score = item["score"].get<double>();
            entry.solved = item["solved"].get<int>();
            entry.streak = item["streak"].get<int>();
            entry.lastActivity = epochToTime(item["lastActivity"]);
            entry.rank = item["rank"].get<int>();
            coll.entries.push_back(entry);
        }
        collections_[key] = std::move(coll);
    }
}

void JsonScoreboardRepository::save() {
    json data = json::object();
    for (std::map<std::string, CollectionData>::iterator it = collections_.begin();
         it != collections_.end(); ++it) {
        json arr = json::array();
        for (size_t e = 0; e < it->second.entries.size(); ++e) {
            const core::ScoreboardEntry& entry = it->second.entries[e];
            json item;
            item["userId"] = entry.userId.get();
            item["name"] = entry.name;
            item["college"] = entry.college;
            item["branch"] = entry.branch;
            item["score"] = entry.score;
            item["solved"] = entry.solved;
            item["streak"] = entry.streak;
            item["lastActivity"] = timeToEpoch(entry.lastActivity);
            item["rank"] = entry.rank;
            arr.push_back(item);
        }
        data[it->first] = arr;
    }

    std::ofstream file(filePath_);
    file << data.dump(2);
}

std::string JsonScoreboardRepository::collectionKey(const std::string& scope,
                                                    const std::string& window,
                                                    const std::string& ranking) const {
    return scope + "_" + window + "_" + ranking;
}

void JsonScoreboardRepository::upsert(const core::ScoreboardEntry& entry,
                                      const std::string& scope,
                                      const std::string& window,
                                      const std::string& ranking) {
    std::string key = collectionKey(scope, window, ranking);
    CollectionData& coll = collections_[key];

    // Find existing entry for this user
    bool found = false;
    for (size_t i = 0; i < coll.entries.size(); ++i) {
        if (coll.entries[i].userId == entry.userId) {
            coll.entries[i] = entry;
            found = true;
            break;
        }
    }
    if (!found) {
        coll.entries.push_back(entry);
    }

    // Re-sort by score and reassign ranks
    std::sort(coll.entries.begin(), coll.entries.end(),
        [](const core::ScoreboardEntry& a, const core::ScoreboardEntry& b) {
            return a.score > b.score;
        });

    for (size_t i = 0; i < coll.entries.size(); ++i) {
        coll.entries[i].rank = static_cast<int>(i) + 1;
    }

    save();
}

std::vector<core::ScoreboardEntry> JsonScoreboardRepository::findTop(int n,
                                                                     const std::string& scope,
                                                                     const std::string& window,
                                                                     const std::string& ranking) {
    std::string key = collectionKey(scope, window, ranking);
    std::map<std::string, CollectionData>::iterator it = collections_.find(key);
    if (it == collections_.end()) {
        return std::vector<core::ScoreboardEntry>();
    }

    std::vector<core::ScoreboardEntry> result = it->second.entries;
    if (static_cast<int>(result.size()) > n) {
        result.resize(static_cast<size_t>(n));
    }
    return result;
}

core::Optional<core::ScoreboardEntry> JsonScoreboardRepository::findUser(const core::UserId& userId,
                                                                         const std::string& scope,
                                                                         const std::string& window,
                                                                         const std::string& ranking) {
    std::string key = collectionKey(scope, window, ranking);
    std::map<std::string, CollectionData>::iterator it = collections_.find(key);
    if (it == collections_.end()) {
        return core::Optional<core::ScoreboardEntry>();
    }

    for (size_t i = 0; i < it->second.entries.size(); ++i) {
        if (it->second.entries[i].userId == userId) {
            return core::Optional<core::ScoreboardEntry>(it->second.entries[i]);
        }
    }
    return core::Optional<core::ScoreboardEntry>();
}

int JsonScoreboardRepository::getUserRank(const core::UserId& userId,
                                          const std::string& scope,
                                          const std::string& window,
                                          const std::string& ranking) {
    std::string key = collectionKey(scope, window, ranking);
    std::map<std::string, CollectionData>::iterator it = collections_.find(key);
    if (it == collections_.end()) {
        return -1;
    }

    for (size_t i = 0; i < it->second.entries.size(); ++i) {
        if (it->second.entries[i].userId == userId) {
            return static_cast<int>(i) + 1;
        }
    }
    return -1;
}

} // namespace db
