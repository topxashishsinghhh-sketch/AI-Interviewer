// OOP: Repository Pattern (JSON Implementation)
#include "db/JsonAttemptRepository.h"
#include "json.hpp"
#include "db/JsonTime.h"
#include <fstream>
#include <chrono>

namespace db {

JsonAttemptRepository::JsonAttemptRepository(const std::string& filePath)
    : filePath_(filePath) {
    // Create directory if needed (fallback for no filesystem)
    size_t pos = filePath.find_last_of("/\\");
    if (pos != std::string::npos) {
        std::string dir = filePath.substr(0, pos);
        #ifdef _WIN32
        std::string cmd = "mkdir \"" + dir + "\" 2>nul";
        #else
        std::string cmd = "mkdir -p \"" + dir + "\"";
        #endif
        system(cmd.c_str());
    }
    load();
}

nlohmann::json JsonAttemptRepository::load() {
    std::ifstream file(filePath_);
    nlohmann::json data;
    if (file.is_open()) {
        file >> data;
        file.close();
    } else {
        data = nlohmann::json::array();
    }
    
    cache_.clear();
    for (const auto& item : data) {
        cache_.push_back(deserialize(item));
    }
    return data;
}

void JsonAttemptRepository::save(const nlohmann::json& data) {
    std::ofstream file(filePath_);
    if (file.is_open()) {
        file << data.dump(2);
        file.close();
    }
}

core::Attempt JsonAttemptRepository::deserialize(const nlohmann::json& j) {
    core::Attempt attempt;
    attempt.id = core::AttemptId(j["id"].get<std::string>());
    attempt.userId = core::UserId(j["userId"].get<std::string>());
    attempt.questionId = core::QuestionId(j["questionId"].get<std::string>());
    attempt.answer = j["answer"].get<std::string>();
    attempt.correct = j["correct"].get<bool>();
    attempt.score = j["score"].get<double>();
    attempt.difficulty = core::fromString(j["difficulty"].get<std::string>());
    attempt.timeTakenSeconds = j["timeTakenSeconds"].get<int>();
    attempt.timestamp = epochToTime(j["timestamp"]);
    attempt.bookmarked = j.value("bookmarked", false);
    attempt.feedback = j.value("feedback", "");
    return attempt;
}

nlohmann::json JsonAttemptRepository::serialize(const core::Attempt& attempt) {
    nlohmann::json j;
    j["id"] = attempt.id.get();
    j["userId"] = attempt.userId.get();
    j["questionId"] = attempt.questionId.get();
    j["answer"] = attempt.answer;
    j["correct"] = attempt.correct;
    j["score"] = attempt.score;
    j["difficulty"] = core::toString(attempt.difficulty);
    j["timeTakenSeconds"] = attempt.timeTakenSeconds;
    j["timestamp"] = timeToEpoch(attempt.timestamp);
    j["bookmarked"] = attempt.bookmarked;
    j["feedback"] = attempt.feedback;
    return j;
}

core::Attempt* JsonAttemptRepository::findById(const core::AttemptId& id) {
    for (std::vector<core::Attempt>::iterator it = cache_.begin(); it != cache_.end(); ++it) {
        if (it->id == id) return &(*it);
    }
    return nullptr;
}

std::vector<core::Attempt> JsonAttemptRepository::findByUser(const core::UserId& userId) {
    std::vector<core::Attempt> result;
    for (std::vector<core::Attempt>::const_iterator it = cache_.begin(); it != cache_.end(); ++it) {
        if (it->userId == userId) result.push_back(*it);
    }
    return result;
}

std::vector<core::Attempt> JsonAttemptRepository::findByUserAndDateRange(
    const core::UserId& userId,
    std::chrono::system_clock::time_point from,
    std::chrono::system_clock::time_point to) {
    std::vector<core::Attempt> result;
    for (std::vector<core::Attempt>::const_iterator it = cache_.begin(); it != cache_.end(); ++it) {
        if (it->userId == userId && it->timestamp >= from && it->timestamp <= to) {
            result.push_back(*it);
        }
    }
    return result;
}

std::vector<core::Attempt> JsonAttemptRepository::findByQuestion(const core::QuestionId& questionId) {
    std::vector<core::Attempt> result;
    for (std::vector<core::Attempt>::const_iterator it = cache_.begin(); it != cache_.end(); ++it) {
        if (it->questionId == questionId) result.push_back(*it);
    }
    return result;
}

void JsonAttemptRepository::save(const core::Attempt& attempt) {
    auto data = load();
    bool found = false;
    for (auto& item : data) {
        if (item["id"] == attempt.id.get()) {
            item = serialize(attempt);
            found = true;
            break;
        }
    }
    if (!found) {
        data.push_back(serialize(attempt));
    }
    save(data);
    // Refresh cache from the persisted state
    load();
}

void JsonAttemptRepository::remove(const core::AttemptId& id) {
    cache_.erase(std::remove_if(cache_.begin(), cache_.end(),
        [&id](const core::Attempt& a) { return a.id == id; }),
        cache_.end());
    auto data = load();
    data.erase(std::remove_if(data.begin(), data.end(),
        [&id](const nlohmann::json& j) { return j["id"] == id.get(); }),
        data.end());
    save(data);
}

} // namespace db