// OOP: Composition (Whole-Part Hierarchy)
#include "core/Course.h"
#include <algorithm>

namespace core {

Topic::Topic(std::string id, std::string name, std::string description)
    : id_(std::move(id)), name_(std::move(name)), description_(std::move(description)) {}

Subject::Subject(std::string id, std::string name, std::string description)
    : id_(std::move(id)), name_(std::move(name)), description_(std::move(description)) {}

void Subject::addTopic(std::shared_ptr<Topic> topic) {
    topics_.push_back(std::move(topic));
}

std::shared_ptr<Topic> Subject::findTopic(const std::string& id) const {
    for (const auto& topic : topics_) {
        if (topic->id() == id) return topic;
    }
    return nullptr;
}

Course::Course(std::string id, std::string name, std::string description)
    : id_(std::move(id)), name_(std::move(name)), description_(std::move(description)) {}

void Course::addSubject(std::shared_ptr<Subject> subject) {
    subjects_.push_back(std::move(subject));
}

std::shared_ptr<Subject> Course::findSubject(const std::string& id) const {
    for (const auto& subject : subjects_) {
        if (subject->id() == id) return subject;
    }
    return nullptr;
}

} // namespace core