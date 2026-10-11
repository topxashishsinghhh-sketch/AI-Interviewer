// OOP: Repository Pattern (JSON Implementation)
#include "db/JsonDailyChallengeRepository.h"
#include "json.hpp"
#include "db/JsonTime.h"
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;

namespace db {

JsonDailyChallengeRepository::JsonDailyChallengeRepository(const std::string& filePath)
    : filePath_(filePath) {
    load();
}

void JsonDailyChallengeRepository::load() {
    std::ifstream file(filePath_);
    if (!file.is_open()) return;

    json data;
    file >> data;

    for (json::iterator it = data.begin(); it != data.end(); ++it) {
        std::string key = it.key();
        const json& item = it.value();
        core::DailyChallenge challenge;
        challenge.courseId = item["courseId"].get<std::string>();
        challenge.date = epochToTime(item["date"]);
        challenge.questionId = core::QuestionId(item["questionId"].get<std::string>());
        challenge.bonusPoints = item["bonusPoints"].get<int>();
        challenges_[key] = challenge;
    }
}

void JsonDailyChallengeRepository::save() {
    json data = json::object();
    for (std::map<std::string, core::DailyChallenge>::iterator it = challenges_.begin();
         it != challenges_.end(); ++it) {
        const core::DailyChallenge& challenge = it->second;
        json item;
        item["courseId"] = challenge.courseId;
        item["date"] = timeToEpoch(challenge.date);
        item["questionId"] = challenge.questionId.get();
        item["bonusPoints"] = challenge.bonusPoints;
        data[it->first] = item;
    }

    std::ofstream file(filePath_);
    file << data.dump(2);
}

std::string JsonDailyChallengeRepository::makeKey(const std::string& courseId,
                                                   std::chrono::system_clock::time_point date) const {
    const std::time_t seconds = std::chrono::system_clock::to_time_t(date);
    const std::tm* tm = std::localtime(&seconds);
    std::ostringstream oss;
    oss << courseId << "_" << (tm->tm_year + 1900) << "-" << std::setw(2) << std::setfill('0') << (tm->tm_mon + 1)
        << "-" << std::setw(2) << std::setfill('0') << tm->tm_mday;
    return oss.str();
}

core::Optional<core::DailyChallenge> JsonDailyChallengeRepository::findByCourseAndDate(const std::string& courseId, 
    std::chrono::system_clock::time_point date) {
    std::string key = makeKey(courseId, date);
    auto it = challenges_.find(key);
    if (it != challenges_.end()) {
        return core::Optional<core::DailyChallenge>(it->second);
    }
    return core::Optional<core::DailyChallenge>();
}

void JsonDailyChallengeRepository::save(const core::DailyChallenge& challenge) {
    std::string key = makeKey(challenge.courseId, challenge.date);
    challenges_[key] = challenge;
    save();
}

std::vector<core::DailyChallenge> JsonDailyChallengeRepository::findByDateRange(
    std::chrono::system_clock::time_point from,
    std::chrono::system_clock::time_point to) {
    std::vector<core::DailyChallenge> result;
    for (std::map<std::string, core::DailyChallenge>::iterator it = challenges_.begin();
         it != challenges_.end(); ++it) {
        if (it->second.date >= from && it->second.date <= to) {
            result.push_back(it->second);
        }
    }
    return result;
}

} // namespace db