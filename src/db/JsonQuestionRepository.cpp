// OOP: Repository Pattern (JSON Implementation)
#include "db/JsonQuestionRepository.h"
#include "core/MCQQuestion.h"
#include "core/TextQuestion.h"
#include "core/CodingQuestion.h"
#include <fstream>

namespace db {

JsonQuestionRepository::JsonQuestionRepository(const std::string& filePath)
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

nlohmann::json JsonQuestionRepository::load() {
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
        auto question = deserialize(item);
        cache_[question->id().get()] = question;
    }
    return data;
}

void JsonQuestionRepository::save(const nlohmann::json& data) {
    std::ofstream file(filePath_);
    if (file.is_open()) {
        file << data.dump(2);
        file.close();
    }
}

core::QuestionPtr JsonQuestionRepository::deserialize(const nlohmann::json& j) {
    core::QuestionId id(j["id"].get<std::string>());
    std::string subject = j["subject"].get<std::string>();
    std::string topic = j["topic"].get<std::string>();
    core::Difficulty difficulty = core::fromString(j["difficulty"].get<std::string>());
    std::string text = j["text"].get<std::string>();
    std::string type = j["type"].get<std::string>();
    std::string explanation = j.value("explanation", "");

    core::QuestionPtr question;
    if (type == "MCQ") {
        std::vector<std::string> options = j["options"].get<std::vector<std::string>>();
        int correctIndex = j["answer"].get<int>();
        question = std::make_shared<core::MCQQuestion>(id, subject, topic, difficulty, text, options, correctIndex, explanation);
    } else if (type == "Text") {
        std::vector<std::string> keywords = j["keywords"].get<std::vector<std::string>>();
        int minKeywords = j.value("minKeywords", 1);
        question = std::make_shared<core::TextQuestion>(id, subject, topic, difficulty, text, keywords, minKeywords, explanation);
    } else if (type == "Coding") {
        std::string templateCode = j["templateCode"].get<std::string>();
        std::vector<core::CodingTestCase> testCases;
        for (const auto& tc : j["testCases"]) {
            testCases.push_back({
                tc["input"].get<std::string>(),
                tc["expectedOutput"].get<std::string>(),
                tc.value("isHidden", false)
            });
        }
        question = std::make_shared<core::CodingQuestion>(id, subject, topic, difficulty, text, templateCode, testCases, explanation);
    }

    // Phase 11: objective, skill tags and keywords are all optional so the
    // existing question files load unchanged.
    if (question) {
        question->setObjective(j.value("objective", ""));
        question->setSkillKind(j.value("softSkill", false) ? core::SkillKind::Soft
                                                           : core::SkillKind::Hard);
        std::vector<std::string> tags;
        if (j.contains("skillTags")) {
            for (const auto& tag : j["skillTags"]) {
                tags.push_back(tag.get<std::string>());
            }
        }
        question->setSkillTags(tags);
    }
    return question;
}

nlohmann::json JsonQuestionRepository::serialize(const core::QuestionPtr& question) {
    nlohmann::json j;
    j["id"] = question->id().get();
    j["subject"] = question->subject();
    j["topic"] = question->topic();
    j["difficulty"] = core::toString(question->difficulty());
    j["text"] = question->text();
    j["type"] = question->type();
    j["explanation"] = question->explanation();
    j["objective"] = question->objective();
    j["softSkill"] = (question->skillKind() == core::SkillKind::Soft);
    j["skillTags"] = question->skillTags();
    
    if (auto mcq = std::dynamic_pointer_cast<core::MCQQuestion>(question)) {
        j["options"] = mcq->options();
        j["answer"] = mcq->correctIndex();
    } else if (auto text = std::dynamic_pointer_cast<core::TextQuestion>(question)) {
        j["keywords"] = text->keywords();
        j["minKeywords"] = text->minKeywords();
    } else if (auto coding = std::dynamic_pointer_cast<core::CodingQuestion>(question)) {
        j["templateCode"] = coding->templateCode();
        j["testCases"] = nlohmann::json::array();
        for (const auto& tc : coding->testCases()) {
            j["testCases"].push_back({
                {"input", tc.input},
                {"expectedOutput", tc.expectedOutput},
                {"isHidden", tc.isHidden}
            });
        }
    }
    
    return j;
}

core::QuestionPtr JsonQuestionRepository::findById(const core::QuestionId& id) {
    auto it = cache_.find(id.get());
    if (it != cache_.end()) return it->second;
    return nullptr;
}

std::vector<core::QuestionPtr> JsonQuestionRepository::findAll() {
    std::vector<core::QuestionPtr> result;
    for (std::unordered_map<std::string, core::QuestionPtr>::const_iterator it = cache_.begin(); it != cache_.end(); ++it) {
        result.push_back(it->second);
    }
    return result;
}

std::vector<core::QuestionPtr> JsonQuestionRepository::findBySubject(const std::string& subject) {
    std::vector<core::QuestionPtr> result;
    for (std::unordered_map<std::string, core::QuestionPtr>::const_iterator it = cache_.begin(); it != cache_.end(); ++it) {
        if (it->second->subject() == subject) result.push_back(it->second);
    }
    return result;
}

std::vector<core::QuestionPtr> JsonQuestionRepository::findBySubjectAndDifficulty(
    const std::string& subject, core::Difficulty difficulty) {
    std::vector<core::QuestionPtr> result;
    for (std::unordered_map<std::string, core::QuestionPtr>::const_iterator it = cache_.begin(); it != cache_.end(); ++it) {
        if (it->second->subject() == subject && it->second->difficulty() == difficulty) result.push_back(it->second);
    }
    return result;
}

std::vector<core::QuestionPtr> JsonQuestionRepository::findByTopic(const std::string& topic) {
    std::vector<core::QuestionPtr> result;
    for (std::unordered_map<std::string, core::QuestionPtr>::const_iterator it = cache_.begin(); it != cache_.end(); ++it) {
        if (it->second->topic() == topic) result.push_back(it->second);
    }
    return result;
}

void JsonQuestionRepository::save(const core::QuestionPtr& question) {
    auto data = load();
    bool found = false;
    for (auto& item : data) {
        if (item["id"] == question->id().get()) {
            item = serialize(question);
            found = true;
            break;
        }
    }
    if (!found) {
        data.push_back(serialize(question));
    }
    save(data);
    // Refresh cache from the persisted state so lookups see the new question
    load();
}

void JsonQuestionRepository::remove(const core::QuestionId& id) {
    cache_.erase(id.get());
    auto data = load();
    data.erase(std::remove_if(data.begin(), data.end(),
        [&id](const nlohmann::json& j) { return j["id"] == id.get(); }),
        data.end());
    save(data);
}

bool JsonQuestionRepository::exists(const core::QuestionId& id) {
    return cache_.find(id.get()) != cache_.end();
}

} // namespace db