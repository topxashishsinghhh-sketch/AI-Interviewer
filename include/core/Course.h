// OOP: Composition (Whole-Part Hierarchy)
#pragma once
#include <string>
#include <vector>
#include <memory>

namespace core {

class Topic {
public:
    Topic(std::string id, std::string name, std::string description = "");
    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }
    const std::string& description() const { return description_; }
private:
    std::string id_;
    std::string name_;
    std::string description_;
};

class Subject {
public:
    Subject(std::string id, std::string name, std::string description = "");
    void addTopic(std::shared_ptr<Topic> topic);
    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }
    const std::string& description() const { return description_; }
    const std::vector<std::shared_ptr<Topic>>& topics() const { return topics_; }
    std::shared_ptr<Topic> findTopic(const std::string& id) const;
private:
    std::string id_;
    std::string name_;
    std::string description_;
    std::vector<std::shared_ptr<Topic>> topics_;
};

class Course {
public:
    Course(std::string id, std::string name, std::string description = "");
    void addSubject(std::shared_ptr<Subject> subject);
    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }
    const std::string& description() const { return description_; }
    const std::vector<std::shared_ptr<Subject>>& subjects() const { return subjects_; }
    std::shared_ptr<Subject> findSubject(const std::string& id) const;
private:
    std::string id_;
    std::string name_;
    std::string description_;
    std::vector<std::shared_ptr<Subject>> subjects_;
};

} // namespace core