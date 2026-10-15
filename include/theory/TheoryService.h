// OOP: Service Layer (theory articles: DSA sheet, core CS and soft skills)
#pragma once
#include "core/IQuestionRepository.h"

#include <memory>
#include <string>
#include <vector>

namespace theory {

// One section of an article.
struct TheorySection {
    std::string heading;
    std::vector<std::string> body;
    std::vector<std::string> bullets;
};

struct TheoryExample {
    std::string title;
    std::string body;
};

// A theory article. "subject" and "topic" tie an article to the practice bank:
// when topic is set, practice questions are counted by topic, otherwise by
// subject. "kind" separates soft skills from technical subjects, and
// "category" groups them in the UI.
struct TheoryArticle {
    std::string id;
    std::string subject;
    std::string topic;
    std::string kind;        // "technical" or "soft"
    std::string category;
    std::string name;
    std::string tagline;
    std::string icon;
    int readingTime = 5;
    std::string summary;
    std::vector<TheorySection> sections;
    std::vector<std::string> keyPoints;
    std::vector<TheoryExample> examples;
    std::vector<std::string> mistakes;
    std::string practiceTip;
};

// Summary row for the section index (no article body).
struct TheorySummary {
    std::string id;
    std::string subject;
    std::string topic;
    std::string kind;
    std::string category;
    std::string name;
    std::string tagline;
    std::string icon;
    int readingTime = 5;
    std::string summary;
    int questionCount = 0;
};

// Loads data/theory.json and exposes it for the web API. The file is content,
// not code, so a new article or a rewording needs no recompile. Practice
// counts come from the question repository so theory and bank cannot disagree.
class TheoryService {
public:
    TheoryService(const std::string& dataDir,
                  std::shared_ptr<core::IQuestionRepository> questionRepo);

    // False when the file is missing or unparsable, so the caller can report
    // a real reason instead of showing an empty section.
    bool loaded() const { return loaded_; }

    // Every article. An empty subject keeps all kinds; otherwise only that
    // subject. Soft-skill articles use their own id as the subject.
    std::vector<TheorySummary> articles(const std::string& subject = "") const;

    // Full article by id; false when the id is unknown.
    bool find(const std::string& id, TheoryArticle& out) const;

    // Practice-question count for one article.
    int questionCountFor(const TheoryArticle& article) const;

private:
    std::string dataDir_;
    std::shared_ptr<core::IQuestionRepository> questionRepo_;
    std::vector<TheoryArticle> articles_;
    bool loaded_ = false;
};

} // namespace theory
