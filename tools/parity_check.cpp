// OOP: Tool (JSON ↔ MongoDB Parity Check)
//
// Compares the JSON repositories against the MongoDB repositories so a cutover
// can be validated before switching STORAGE_BACKEND to mongo.
#include "db/Database.h"
#include "db/MongoUserRepository.h"
#include "db/MongoQuestionRepository.h"
#include "db/MongoAttemptRepository.h"
#include "db/JsonUserRepository.h"
#include "db/JsonQuestionRepository.h"
#include "db/JsonAttemptRepository.h"
#include "core/StrongId.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

namespace {

int failures = 0;

void check(const std::string& label, long long mongo, long long json) {
    const bool ok = (mongo == json);
    std::cout << "  " << (ok ? "PASS" : "FAIL") << "  " << label
              << ": mongo=" << mongo << " json=" << json << std::endl;
    if (!ok) {
        ++failures;
    }
}

} // namespace

int main(int argc, char* argv[]) {
    std::string uri = "mongodb://localhost:27017";
    std::string dbName = "ai_interviewer";
    std::string dataDir = "./data";

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--uri" && i + 1 < argc) uri = argv[++i];
        else if (arg == "--db" && i + 1 < argc) dbName = argv[++i];
        else if (arg == "--data-dir" && i + 1 < argc) dataDir = argv[++i];
    }

    try {
        std::cout << "[parity_check] connecting to " << uri << " (db " << dbName << ")" << std::endl;
        db::Database::instance().initialize(uri, dbName);

        db::MongoUserRepository mongoUsers(dbName);
        db::MongoQuestionRepository mongoQuestions(dbName);
        db::MongoAttemptRepository mongoAttempts(dbName);

        db::JsonUserRepository jsonUsers(dataDir + "/users.json");
        db::JsonQuestionRepository jsonQuestions(dataDir + "/questions.json");
        db::JsonAttemptRepository jsonAttempts(dataDir + "/attempts.json");

        const std::vector<core::UserPtr> mongoUserList = mongoUsers.findAll();
        const std::vector<core::UserPtr> jsonUserList = jsonUsers.findAll();
        const std::vector<core::QuestionPtr> mongoQuestionList = mongoQuestions.findAll();
        const std::vector<core::QuestionPtr> jsonQuestionList = jsonQuestions.findAll();

        std::cout << "[parity_check] record counts" << std::endl;
        check("users", static_cast<long long>(mongoUserList.size()),
              static_cast<long long>(jsonUserList.size()));
        check("questions", static_cast<long long>(mongoQuestionList.size()),
              static_cast<long long>(jsonQuestionList.size()));

        // Per-subject and per-difficulty breakdown, which is where a partial
        // seed usually shows up first.
        std::cout << "[parity_check] questions by subject" << std::endl;
        std::vector<std::string> subjects;
        for (const core::QuestionPtr& q : mongoQuestionList) {
            if (std::find(subjects.begin(), subjects.end(), q->subject()) == subjects.end()) {
                subjects.push_back(q->subject());
            }
        }
        for (const core::QuestionPtr& q : jsonQuestionList) {
            if (std::find(subjects.begin(), subjects.end(), q->subject()) == subjects.end()) {
                subjects.push_back(q->subject());
            }
        }

        const core::Difficulty difficulties[] = {
            core::Difficulty::Easy, core::Difficulty::Medium, core::Difficulty::Hard
        };
        for (const std::string& subject : subjects) {
            for (const core::Difficulty difficulty : difficulties) {
                const std::size_t m = mongoQuestions.findBySubjectAndDifficulty(subject, difficulty).size();
                const std::size_t j = jsonQuestions.findBySubjectAndDifficulty(subject, difficulty).size();
                check(subject + "/" + core::toString(difficulty),
                      static_cast<long long>(m), static_cast<long long>(j));
            }
        }

        std::cout << "[parity_check] attempts" << std::endl;
        std::size_t mongoAttemptCount = 0;
        for (const core::UserPtr& user : mongoUserList) {
            mongoAttemptCount += mongoAttempts.findByUser(user->id()).size();
        }
        std::size_t jsonAttemptCount = 0;
        for (const core::UserPtr& user : jsonUserList) {
            jsonAttemptCount += jsonAttempts.findByUser(user->id()).size();
        }
        check("attempts", static_cast<long long>(mongoAttemptCount),
              static_cast<long long>(jsonAttemptCount));

        if (failures > 0) {
            std::cout << "[parity_check] " << failures << " mismatch(es) found" << std::endl;
            return 1;
        }
        std::cout << "[parity_check] all counts match" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[parity_check] error: " << e.what() << std::endl;
        return 1;
    }
}
