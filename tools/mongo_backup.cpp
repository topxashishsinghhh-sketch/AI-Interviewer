// OOP: Tool (MongoDB Backup)
//
// Dumps every collection the application owns into a timestamped directory as
// extended JSON, plus a manifest describing what was written.
#include "db/Database.h"
#include "db/MongoUserRepository.h"
#include "db/MongoQuestionRepository.h"
#include "db/BSONMappers.h"
#include "core/StrongId.h"

#include <bsoncxx/json.hpp>

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

void writeJsonFile(const std::string& path, const std::string& body) {
    std::ofstream out(path.c_str());
    if (!out.is_open()) {
        throw std::runtime_error("cannot write " + path);
    }
    out << body;
}

// Renders each document as one line of extended JSON so the result is both
// diff-friendly and directly reloadable by mongoimport / mongorestore.
void appendLines(const std::vector<bsoncxx::document::value>& docs,
                 const std::string& path,
                 std::size_t& count) {
    std::ofstream out(path.c_str());
    if (!out.is_open()) {
        throw std::runtime_error("cannot write " + path);
    }
    count = 0;
    for (const bsoncxx::document::value& doc : docs) {
        out << bsoncxx::to_json(doc.view()) << "\n";
        ++count;
    }
}

} // namespace

int main(int argc, char* argv[]) {
    std::string uri = "mongodb://localhost:27017";
    std::string dbName = "ai_interviewer";
    std::string outputDir = "./backups";

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--uri" && i + 1 < argc) uri = argv[++i];
        else if (arg == "--db" && i + 1 < argc) dbName = argv[++i];
        else if (arg == "--output" && i + 1 < argc) outputDir = argv[++i];
    }

    try {
        std::cout << "[mongo_backup] connecting to " << uri << " (db " << dbName << ")" << std::endl;
        db::Database::instance().initialize(uri, dbName);

        // Timestamp for the backup directory.
        const std::time_t now = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        std::tm tm = *std::localtime(&now);
        std::ostringstream stamp;
        stamp << std::put_time(&tm, "%Y%m%d_%H%M%S");
        const std::string timestamp = stamp.str();

        const std::string backupDir = outputDir + "/backup_" + timestamp;

        db::MongoUserRepository userRepo(dbName);
        db::MongoQuestionRepository questionRepo(dbName);

        std::size_t userCount = 0;
        std::size_t questionCount = 0;

        writeJsonFile(backupDir + "/.keep", "");

        {
            std::vector<bsoncxx::document::value> docs;
            const std::vector<core::UserPtr> users = userRepo.findAll();
            for (const core::UserPtr& user : users) {
                docs.push_back(db::UserMapper::toBSON(user));
            }
            appendLines(docs, backupDir + "/users.json", userCount);
        }

        {
            std::vector<bsoncxx::document::value> docs;
            const std::vector<core::QuestionPtr> questions = questionRepo.findAll();
            for (const core::QuestionPtr& question : questions) {
                docs.push_back(db::QuestionMapper::toBSON(question));
            }
            appendLines(docs, backupDir + "/questions.json", questionCount);
        }

        std::ostringstream manifest;
        manifest << "{\n"
                 << "  \"timestamp\": \"" << timestamp << "\",\n"
                 << "  \"database\": \"" << dbName << "\",\n"
                 << "  \"users\": " << userCount << ",\n"
                 << "  \"questions\": " << questionCount << "\n"
                 << "}\n";
        writeJsonFile(backupDir + "/manifest.json", manifest.str());

        std::cout << "[mongo_backup] backup complete: " << backupDir << std::endl;
        std::cout << "  Users: " << userCount << std::endl;
        std::cout << "  Questions: " << questionCount << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[mongo_backup] error: " << e.what() << std::endl;
        return 1;
    }
}
