// OOP: Service Layer (soft-skills theory content)
#include "softskills/SoftSkillsService.h"
#include "core/IQuestionRepository.h"

#include <fstream>
#include <sstream>
#include "json.hpp"

namespace softskills {

namespace {

// Reads the whole file; an unreadable file yields an empty optional-style
// result that the caller treats as "not loaded".
bool readFile(const std::string& path, std::string& out) {
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in.is_open()) return false;
    std::ostringstream buffer;
    buffer << in.rdbuf();
    out = buffer.str();
    return true;
}

std::string stringAt(const nlohmann::json& object, const char* key, const std::string& fallback = "") {
    if (!object.is_object()) return fallback;
    const auto it = object.find(key);
    if (it == object.end() || !it->is_string()) return fallback;
    return it->get<std::string>();
}

int intAt(const nlohmann::json& object, const char* key, int fallback = 0) {
    if (!object.is_object()) return fallback;
    const auto it = object.find(key);
    if (it == object.end() || !it->is_number()) return fallback;
    return it->get<int>();
}

std::vector<std::string> stringArrayAt(const nlohmann::json& object, const char* key) {
    std::vector<std::string> values;
    if (!object.is_object()) return values;
    const auto it = object.find(key);
    if (it == object.end() || !it->is_array()) return values;
    for (std::size_t i = 0; i < it->size(); ++i) {
        if ((*it)[i].is_string()) values.push_back((*it)[i].get<std::string>());
    }
    return values;
}

TheorySection parseSection(const nlohmann::json& object) {
    TheorySection section;
    section.heading = stringAt(object, "heading");
    section.body = stringArrayAt(object, "body");
    section.bullets = stringArrayAt(object, "bullets");
    return section;
}

TheoryExample parseExample(const nlohmann::json& object) {
    TheoryExample example;
    example.title = stringAt(object, "title");
    example.body = stringAt(object, "body");
    return example;
}

} // namespace

SoftSkillsService::SoftSkillsService(const std::string& dataDir,
                                     std::shared_ptr<core::IQuestionRepository> questionRepo)
    : dataDir_(dataDir)
    , questionRepo_(std::move(questionRepo)) {
    std::string text;
    const std::string path = dataDir_ + "/soft_skills_theory.json";
    if (!readFile(path, text)) {
        return;
    }

    nlohmann::json document;
    try {
        document = nlohmann::json::parse(text);
    } catch (const std::exception&) {
        return;
    }

    if (!document.is_array()) {
        return;
    }

    for (std::size_t i = 0; i < document.size(); ++i) {
        const nlohmann::json& item = document[i];
        TheorySubject subject;
        subject.id = stringAt(item, "id");
        subject.name = stringAt(item, "name");
        subject.tagline = stringAt(item, "tagline");
        subject.icon = stringAt(item, "icon");
        subject.readingTime = intAt(item, "readingTime", 5);
        subject.summary = stringAt(item, "summary");
        subject.keyPoints = stringArrayAt(item, "keyPoints");
        subject.mistakes = stringArrayAt(item, "mistakes");
        subject.practiceTip = stringAt(item, "practiceTip");

        if (item.is_object()) {
            const auto sections = item.find("sections");
            if (sections != item.end() && sections->is_array()) {
                for (std::size_t s = 0; s < sections->size(); ++s) {
                    subject.sections.push_back(parseSection((*sections)[s]));
                }
            }
            const auto examples = item.find("examples");
            if (examples != item.end() && examples->is_array()) {
                for (std::size_t e = 0; e < examples->size(); ++e) {
                    subject.examples.push_back(parseExample((*examples)[e]));
                }
            }
        }

        if (subject.id.empty()) continue;
        subjects_.push_back(subject);
    }

    loaded_ = !subjects_.empty();
}

int SoftSkillsService::questionCountFor(const std::string& subjectId) const {
    if (!questionRepo_) return 0;
    return static_cast<int>(questionRepo_->findBySubject(subjectId).size());
}

std::vector<TheorySummary> SoftSkillsService::subjects() const {
    std::vector<TheorySummary> summaries;
    summaries.reserve(subjects_.size());
    for (std::size_t i = 0; i < subjects_.size(); ++i) {
        const TheorySubject& subject = subjects_[i];
        TheorySummary summary;
        summary.id = subject.id;
        summary.name = subject.name;
        summary.tagline = subject.tagline;
        summary.icon = subject.icon;
        summary.readingTime = subject.readingTime;
        summary.summary = subject.summary;
        summary.questionCount = questionCountFor(subject.id);
        summaries.push_back(summary);
    }
    return summaries;
}

bool SoftSkillsService::find(const std::string& id, TheorySubject& out) const {
    for (std::size_t i = 0; i < subjects_.size(); ++i) {
        if (subjects_[i].id == id) {
            out = subjects_[i];
            return true;
        }
    }
    return false;
}

} // namespace softskills
