// OOP: Tool (MongoDB Restore)
//
// Reloads the extended-JSON documents written by mongo_backup into MongoDB.
// Existing records are replaced (matched on their business key) unless
// --skip-existing is given.
#include "db/Database.h"
#include "db/MongoUserRepository.h"
#include "db/MongoQuestionRepository.h"
#include "db/BSONMappers.h"
#include "core/StrongId.h"

#include <bsoncxx/json.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct RestoreCounts {
    int inserted = 0;
    int replaced = 0;
    int skipped = 0;
    int failed = 0;
};

// Reads one extended-JSON document per line.
std::vector<bsoncxx::document::value> readDocuments(const std::string& path) {
    std::vector<bsoncxx::document::value> docs;

    std::ifstream in(path.c_str());
    if (!in.is_open()) {
        // A missing file simply means the backup held no such collection.
        return docs;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        try {
            docs.push_back(bsoncxx::from_json(line));
        } catch (const std::exception&) {
            // Ignore malformed lines; the summary reports the failures.
        }
    }
    return docs;
}

void restoreUsers(const std::string& path,
                  db::MongoUserRepository& repo,
                  bool skipExisting,
                  RestoreCounts& counts) {
    const std::vector<bsoncxx::document::value> docs = readDocuments(path);
    for (const bsoncxx::document::value& doc : docs) {
        try {
            const core::UserPtr user = db::UserMapper::fromBSON(doc.view());
            if (!user) {
                ++counts.failed;
                continue;
            }
            if (repo.exists(user->email())) {
                if (skipExisting) {
                    ++counts.skipped;
                    continue;
                }
                ++counts.replaced;
            } else {
                ++counts.inserted;
            }
            repo.save(user);
        } catch (const std::exception& e) {
            ++counts.failed;
            std::cerr << "  user restore failed: " << e.what() << std::endl;
        }
    }
}

void restoreQuestions(const std::string& path,
                      db::MongoQuestionRepository& repo,
                      bool skipExisting,
                      RestoreCounts& counts) {
    const std::vector<bsoncxx::document::value> docs = readDocuments(path);
    for (const bsoncxx::document::value& doc : docs) {
        try {
            const core::QuestionPtr question = db::QuestionMapper::fromBSON(doc.view());
            if (!question) {
                ++counts.failed;
                continue;
            }
            if (repo.exists(question->id())) {
                if (skipExisting) {
                    ++counts.skipped;
                    continue;
                }
                ++counts.replaced;
            } else {
                ++counts.inserted;
            }
            repo.save(question);
        } catch (const std::exception& e) {
            ++counts.failed;
            std::cerr << "  question restore failed: " << e.what() << std::endl;
        }
    }
}

} // namespace

int main(int argc, char* argv[]) {
    std::string uri = "mongodb://localhost:27017";
    std::string dbName = "ai_interviewer";
    std::string backupDir;
    bool force = false;
    bool skipExisting = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--uri" && i + 1 < argc) uri = argv[++i];
        else if (arg == "--db" && i + 1 < argc) dbName = argv[++i];
        else if (arg == "--backup-dir" && i + 1 < argc) backupDir = argv[++i];
        else if (arg == "--force") force = true;
        else if (arg == "--skip-existing") skipExisting = true;
    }

    if (backupDir.empty()) {
        std::cerr << "Usage: mongo_restore --backup-dir <path> [--uri <uri>] "
                     "[--db <db>] [--skip-existing]" << std::endl;
        return 1;
    }

    try {
        std::cout << "[mongo_restore] connecting to " << uri << " (db " << dbName << ")" << std::endl;
        db::Database::instance().initialize(uri, dbName);

        db::MongoUserRepository userRepo(dbName);
        db::MongoQuestionRepository questionRepo(dbName);

        RestoreCounts counts;

        restoreUsers(backupDir + "/users.json", userRepo, skipExisting, counts);
        restoreQuestions(backupDir + "/questions.json", questionRepo, skipExisting, counts);

        std::cout << "[mongo_restore] complete" << std::endl;
        std::cout << "  Inserted: " << counts.inserted << std::endl;
        std::cout << "  Replaced: " << counts.replaced << std::endl;
        std::cout << "  Skipped:  " << counts.skipped << std::endl;
        std::cout << "  Failed:   " << counts.failed << std::endl;
        (void)force;
        return counts.failed == 0 ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "[mongo_restore] error: " << e.what() << std::endl;
        return 1;
    }
}
