// OOP: Service Layer (Business Logic Facade)
#pragma once
#include "core/Course.h"
#include "core/User.h"
#include <vector>
#include <memory>

namespace course {

class CourseService {
public:
    CourseService();
    
    const std::vector<std::shared_ptr<core::Course>>& courses() const;
    std::shared_ptr<core::Course> findCourse(const std::string& id) const;
    std::shared_ptr<core::Subject> findSubject(const std::string& courseId, const std::string& subjectId) const;
    std::shared_ptr<core::Topic> findTopic(const std::string& courseId, const std::string& subjectId, const std::string& topicId) const;
    
    std::vector<std::shared_ptr<core::Subject>> subjectsForCourse(const std::string& courseId) const;
    std::vector<std::shared_ptr<core::Topic>> topicsForSubject(const std::string& courseId, const std::string& subjectId) const;
    
    bool isUnlocked(const core::UserPtr& user, const std::string& courseId, const std::string& subjectId, const std::string& topicId) const;
    
private:
    void initializeCourses();
    std::vector<std::shared_ptr<core::Course>> courses_;
};

} // namespace course