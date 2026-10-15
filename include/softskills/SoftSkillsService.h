// OOP: Service Layer (soft-skills theory content)
#pragma once
#include "core/IQuestionRepository.h"

#include <memory>
#include <string>
#include <vector>

namespace softskills {

// One section of a theory article.
struct TheorySection {
    std::string heading;
    std::vector<std::string> body;
    std::vector<std::string> bullets;
};

struct TheoryExample {
    std::string title;
    std::string body;
};

// A soft-skill subject: metadata plus its readable theory article.
struct TheorySubject {
    std::string id;
    std::string name;
    std::string tagline;
    std::string icon;
    int readingTime = 0;
    std::string summary;
    std::vector<TheorySection> sections;
    std::vector<std::string> keyPoints;
    std::vector<TheoryExample> examples;
    std::vector<std::string> mistakes;
    std::string practiceTip;
};

// Summary row used by the section index (no article body).
struct TheorySummary {
    std::string id;
    std::string name;
    std::string tagline;
    std::string icon;
    int readingTime = 0;
    std::string summary;
    int questionCount = 0;
};

// Loads data/soft_skills_theory.json and exposes it for the web API. The file
// is content, not code, so a new subject or a reworded article needs no
// recompile. Practice-question counts come from the question repository, so
// the theory and the bank can never disagree.
class SoftSkillsService {
public:
    SoftSkillsService(const std::string& dataDir,
                      std::shared_ptr<core::IQuestionRepository> questionRepo);

    // False when the theory file is missing or unparsable, so the caller can
    // report a real reason instead of showing an empty section.
    bool loaded() const { return loaded_; }

    // Every soft-skill subject, ordered as declared in the data file.
    std::vector<TheorySummary> subjects() const;

    // Full article for one subject; false when the id is unknown.
    bool find(const std::string& id, TheorySubject& out) const;

private:
    int questionCountFor(const std::string& subjectId) const;

    std::string dataDir_;
    std::shared_ptr<core::IQuestionRepository> questionRepo_;
    std::vector<TheorySubject> subjects_;
    bool loaded_ = false;
};

} // namespace softskills
