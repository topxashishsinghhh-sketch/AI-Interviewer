// OOP: Repository Pattern (JSON Implementation)
#include "db/JsonUserRepository.h"
#include "core/Student.h"
#include <fstream>

namespace db {

JsonUserRepository::JsonUserRepository(const std::string& filePath)
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
    load(); // Initialize cache
}

nlohmann::json JsonUserRepository::load() {
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
        auto user = deserialize(item);
        cache_[user->id().get()] = user;
    }
    return data;
}

void JsonUserRepository::save(const nlohmann::json& data) {
    std::ofstream file(filePath_);
    if (file.is_open()) {
        file << data.dump(2);
        file.close();
    }
}

core::UserPtr JsonUserRepository::deserialize(const nlohmann::json& j) {
    core::UserId id(j["id"].get<std::string>());
    std::string email = j["email"].get<std::string>();
    std::string passwordHash = j["passwordHash"].get<std::string>();
    std::string name = j["name"].get<std::string>();
    std::string roleStr = j["role"].get<std::string>();
    std::string college = j.value("college", "Unspecified");
    std::string branch = j.value("branch", "Unspecified");
    
    core::UserRole role = (roleStr == "Admin") ? core::UserRole::Admin : core::UserRole::Student;
    
    core::UserPtr user;
    if (role == core::UserRole::Admin) {
        user = std::make_shared<core::Admin>(id, email, passwordHash, name, college, branch);
    } else {
        user = std::make_shared<core::Student>(id, email, passwordHash, name, college, branch);
        auto student = std::dynamic_pointer_cast<core::Student>(user);
        if (student) {
            if (j.contains("streak")) student->setStreak(j["streak"].get<int>());
            if (j.contains("longestStreak")) student->setLongestStreak(j["longestStreak"].get<int>());
            if (j.contains("totalSolved")) student->setTotalSolved(j["totalSolved"].get<int>());
            if (j.contains("totalAttempts")) student->setTotalAttempts(j["totalAttempts"].get<int>());
            if (j.contains("lastActivityDate")) student->setLastActivityDate(j["lastActivityDate"].get<std::string>());
            if (j.contains("badges")) {
                for (const auto& badge : j["badges"]) {
                    student->addBadge(badge.get<std::string>());
                }
            }
            if (j.contains("bookmarkedQuestions")) {
                for (const auto& qid : j["bookmarkedQuestions"]) {
                    student->bookmarkQuestion(core::QuestionId(qid.get<std::string>()));
                }
            }
        }
    }
    
    return user;
}

nlohmann::json JsonUserRepository::serialize(const core::UserPtr& user) {
    nlohmann::json j;
    j["id"] = user->id().get();
    j["email"] = user->email();
    j["passwordHash"] = user->passwordHash();
    j["name"] = user->name();
    j["role"] = user->isAdmin() ? "Admin" : "Student";
    j["college"] = user->college();
    j["branch"] = user->branch();
    
    if (auto student = std::dynamic_pointer_cast<core::Student>(user)) {
        j["streak"] = student->streak();
        j["longestStreak"] = student->longestStreak();
        j["totalSolved"] = student->totalSolved();
        j["totalAttempts"] = student->totalAttempts();
        j["lastActivityDate"] = student->lastActivityDate();
        j["badges"] = student->badges();
        j["bookmarkedQuestions"] = nlohmann::json::array();
        for (const auto& qid : student->bookmarkedQuestions()) {
            j["bookmarkedQuestions"].push_back(qid.get());
        }
    }
    
    return j;
}

core::UserPtr JsonUserRepository::findById(const core::UserId& id) {
    auto it = cache_.find(id.get());
    if (it != cache_.end()) return it->second;
    return nullptr;
}

core::UserPtr JsonUserRepository::findByEmail(const std::string& email) {
    for (const auto& kv : cache_) {
        if (kv.second->email() == email) return kv.second;
    }
    return nullptr;
}

std::vector<core::UserPtr> JsonUserRepository::findAll() {
    std::vector<core::UserPtr> result;
    for (std::unordered_map<std::string, core::UserPtr>::const_iterator it = cache_.begin(); it != cache_.end(); ++it) {
        result.push_back(it->second);
    }
    return result;
}

void JsonUserRepository::save(const core::UserPtr& user) {
    auto data = load();
    bool found = false;
    for (auto& item : data) {
        if (item["id"] == user->id().get()) {
            item = serialize(user);
            found = true;
            break;
        }
    }
    if (!found) {
        data.push_back(serialize(user));
    }
    save(data);
    // Refresh cache from the persisted state so lookups see the new user
    load();
}

void JsonUserRepository::remove(const core::UserId& id) {
    cache_.erase(id.get());
    auto data = load();
    data.erase(std::remove_if(data.begin(), data.end(),
        [&id](const nlohmann::json& j) { return j["id"] == id.get(); }),
        data.end());
    save(data);
}

bool JsonUserRepository::exists(const std::string& email) {
    return findByEmail(email) != nullptr;
}

} // namespace db