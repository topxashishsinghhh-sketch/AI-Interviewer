// OOP: Builder Pattern (Complex Object Construction)
#pragma once
#include "core/InterviewSession.h"
#include "core/Question.h"
#include "core/User.h"
#include <vector>
#include <memory>
#include <string>

namespace interview {

class InterviewSessionBuilder {
public:
    InterviewSessionBuilder& withId(const core::SessionId& id);
    InterviewSessionBuilder& withUser(const core::UserPtr& user);
    InterviewSessionBuilder& withMode(core::InterviewSession::Mode mode);
    InterviewSessionBuilder& withQuestions(std::vector<core::QuestionPtr> questions);
    InterviewSessionBuilder& addQuestion(const core::QuestionPtr& question);
    
    std::shared_ptr<core::InterviewSession> build();
    
private:
    core::SessionId id_;
    core::UserPtr user_;
    core::InterviewSession::Mode mode_ = core::InterviewSession::Mode::Practice;
    std::vector<core::QuestionPtr> questions_;
};

} // namespace interview