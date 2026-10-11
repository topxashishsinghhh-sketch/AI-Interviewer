// OOP: Tool (Database Seeding)
#include "db/Database.h"
#include "db/MongoUserRepository.h"
#include "db/MongoQuestionRepository.h"
#include "db/BSONMappers.h"
#include "core/Student.h"
#include "auth/AuthService.h"

#include <bsoncxx/json.hpp>
#include <json.hpp>

#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>

using json = nlohmann::json;

namespace {

struct SeedCounts {
    int inserted = 0;
    int updated = 0;
    int skipped = 0;
    int failed = 0;
};

// Reads an environment variable, returning fallback when it is unset/empty.
std::string envVar(const char* name, const std::string& fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') return fallback;
    return std::string(value);
}

// Imports every question in the merged question bank file.
void seedQuestions(const std::string& filePath,
                   db::MongoQuestionRepository& questionRepo,
                   bool verbose,
                   SeedCounts& counts) {
    std::ifstream file(filePath.c_str());
    if (!file.is_open()) {
        throw std::runtime_error("cannot open question bank: " + filePath);
    }

    json data = json::parse(file);
    if (!data.is_array()) {
        throw std::runtime_error("question bank must be a JSON array: " + filePath);
    }

    for (const auto& item : data) {
        try {
            // The JSON banks spell the identifier "id"; MongoDB documents and
            // the mappers use "_id", so normalise it here.
            json entry = item;
            if (entry.contains("id") && !entry.contains("_id")) {
                entry["_id"] = entry["id"];
            }
            const bsoncxx::document::value doc = bsoncxx::from_json(entry.dump());
            const core::QuestionPtr question =
                db::QuestionMapper::fromBSON(doc.view());
            if (!question) {
                ++counts.skipped;
                continue;
            }

            const bool exists = questionRepo.exists(question->id());
            questionRepo.save(question);
            if (exists) {
                ++counts.updated;
                if (verbose) {
                    std::cout << "  Updated: " << question->id().get() << std::endl;
                }
            } else {
                ++counts.inserted;
                if (verbose) {
                    std::cout << "  Inserted: " << question->id().get() << std::endl;
                }
            }
        } catch (const std::exception& e) {
            ++counts.failed;
            std::cerr << "  Failed: " << e.what() << std::endl;
        }
    }
}

// Creates the bootstrap admin account when it is missing.
void seedAdmin(const std::shared_ptr<core::IUserRepository>& userRepo, SeedCounts& counts) {
    // Credentials come from the environment so no default password is committed.
    // The fallbacks are local-development values only; anything reachable by
    // other people must set SEED_ADMIN_EMAIL / SEED_ADMIN_PASSWORD.
    const std::string email = envVar("SEED_ADMIN_EMAIL", "admin@interviewer.local");
    const std::string password = envVar("SEED_ADMIN_PASSWORD", "ChangeMe!Local");

    if (password == "ChangeMe!Local") {
        std::cout << "[seed_db] note: set SEED_ADMIN_PASSWORD to override the local "
                     "default password" << std::endl;
    }

    if (userRepo->findByEmail(email)) {
        std::cout << "[seed_db] admin account already present for " << email << std::endl;
        return;
    }

    auth::AuthService auth(userRepo);
    const auth::AuthResult registration = auth.registerUser(
        email, password, "Admin User", core::UserRole::Admin, "System", "Admin");
    if (!registration.success) {
        throw std::runtime_error("could not create admin account: " + registration.message);
    }
    ++counts.inserted;
    std::cout << "[seed_db] Created admin user " << email << std::endl;
}

} // namespace

int main(int argc, char* argv[]) {
    std::string uri = "mongodb://localhost:27017";
    std::string dbName = "ai_interviewer";
    std::string questionFile = "./data/questions.json";
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--uri" && i + 1 < argc) {
            uri = argv[++i];
        } else if (arg == "--db" && i + 1 < argc) {
            dbName = argv[++i];
        } else if (arg == "--questions" && i + 1 < argc) {
            questionFile = argv[++i];
        } else if (arg == "-v") {
            verbose = true;
        }
    }

    try {
        std::cout << "[seed_db] connecting to " << uri << " (db " << dbName << ")" << std::endl;
        db::Database::instance().initialize(uri, dbName);

        const std::shared_ptr<core::IUserRepository> userRepo =
            std::make_shared<db::MongoUserRepository>(dbName);
        db::MongoQuestionRepository questionRepo(dbName);

        SeedCounts counts;

        std::cout << "[seed_db] loading questions from " << questionFile << "..." << std::endl;
        seedQuestions(questionFile, questionRepo, verbose, counts);

        seedAdmin(userRepo, counts);

        std::cout << "[seed_db] complete" << std::endl;
        std::cout << "  Inserted: " << counts.inserted << std::endl;
        std::cout << "  Updated:  " << counts.updated << std::endl;
        std::cout << "  Skipped:  " << counts.skipped << std::endl;
        std::cout << "  Failed:   " << counts.failed << std::endl;
        return counts.failed == 0 ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "[seed_db] error: " << e.what() << std::endl;
        return 1;
    }
}
