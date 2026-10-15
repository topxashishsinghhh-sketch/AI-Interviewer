// OOP: Service Layer (theory articles: DSA sheet, core CS and soft skills)
#include "theory/TheoryService.h"
#include "core/IQuestionRepository.h"

#include <fstream>
#include <sstream>
#include "json.hpp"

namespace theory {

namespace {

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

int intAt(const nlohmann::json& object, const char* key, int fallback = 5) {
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

TheoryService::TheoryService(const std::string& dataDir,
                             std::shared_ptr<core::IQuestionRepository> questionRepo)
    : dataDir_(dataDir)
    , questionRepo_(std::move(questionRepo)) {
    std::string text;
    const std::string path = dataDir_ + "/theory.json";
    if (!readFile(path, text)) return;

    nlohmann::json document;
    try {
        document = nlohmann::json::parse(text);
    } catch (const std::exception&) {
        return;
    }

    if (!document.is_array()) return;

    for (std::size_t i = 0; i < document.size(); ++i) {
        const nlohmann::json& item = document[i];
        TheoryArticle article;
        article.id = stringAt(item, "id");
        article.subject = stringAt(item, "subject");
        article.topic = stringAt(item, "topic");
        article.kind = stringAt(item, "kind", "technical");
        article.category = stringAt(item, "category");
        article.name = stringAt(item, "name");
        article.tagline = stringAt(item, "tagline");
        article.icon = stringAt(item, "icon");
        article.readingTime = intAt(item, "readingTime");
        article.summary = stringAt(item, "summary");
        article.keyPoints = stringArrayAt(item, "keyPoints");
        article.mistakes = stringArrayAt(item, "mistakes");
        article.practiceTip = stringAt(item, "practiceTip");

        if (item.is_object()) {
            const auto sections = item.find("sections");
            if (sections != item.end() && sections->is_array()) {
                for (std::size_t s = 0; s < sections->size(); ++s) {
                    article.sections.push_back(parseSection((*sections)[s]));
                }
            }
            const auto examples = item.find("examples");
            if (examples != item.end() && examples->is_array()) {
                for (std::size_t e = 0; e < examples->size(); ++e) {
                    article.examples.push_back(parseExample((*examples)[e]));
                }
            }
        }

        // Soft-skill articles use their own id as the subject.
        if (article.subject.empty()) article.subject = article.id;
        if (article.id.empty()) continue;

        articles_.push_back(article);
    }

    loaded_ = !articles_.empty();
}

int TheoryService::questionCountFor(const TheoryArticle& article) const {
    if (!questionRepo_) return 0;
    if (!article.topic.empty()) {
        const std::vector<core::QuestionPtr> byTopic = questionRepo_->findByTopic(article.topic);
        if (!byTopic.empty()) return static_cast<int>(byTopic.size());
    }
    return static_cast<int>(questionRepo_->findBySubject(article.subject).size());
}

std::vector<TheorySummary> TheoryService::articles(const std::string& subject) const {
    std::vector<TheorySummary> summaries;
    summaries.reserve(articles_.size());
    for (std::size_t i = 0; i < articles_.size(); ++i) {
        const TheoryArticle& article = articles_[i];
        if (!subject.empty() &&
            article.subject != subject &&
            article.id != subject &&
            article.category != subject) {
            continue;
        }

        TheorySummary summary;
        summary.id = article.id;
        summary.subject = article.subject;
        summary.topic = article.topic;
        summary.kind = article.kind;
        summary.category = article.category;
        summary.name = article.name;
        summary.tagline = article.tagline;
        summary.icon = article.icon;
        summary.readingTime = article.readingTime;
        summary.summary = article.summary;
        summary.questionCount = questionCountFor(article);
        summaries.push_back(summary);
    }
    return summaries;
}

bool TheoryService::find(const std::string& id, TheoryArticle& out) const {
    for (std::size_t i = 0; i < articles_.size(); ++i) {
        if (articles_[i].id == id) {
            out = articles_[i];
            return true;
        }
    }
    return false;
}

} // namespace theory
