// OOP: Builder Pattern (Complex Object Construction)
#include "interview/InterviewSessionBuilder.h"
#include "core/InterviewSession.h"
#include <memory>

namespace interview {

InterviewSessionBuilder& InterviewSessionBuilder::withId(const core::SessionId& id) {
    id_ = id;
    return *this;
}

InterviewSessionBuilder& InterviewSessionBuilder::withUser(const core::UserPtr& user) {
    user_ = user;
    return *this;
}

InterviewSessionBuilder& InterviewSessionBuilder::withMode(core::InterviewSession::Mode mode) {
    mode_ = mode;
    return *this;
}

InterviewSessionBuilder& InterviewSessionBuilder::withQuestions(std::vector<core::QuestionPtr> questions) {
    questions_ = std::move(questions);
    return *this;
}

InterviewSessionBuilder& InterviewSessionBuilder::addQuestion(const core::QuestionPtr& question) {
    questions_.push_back(question);
    return *this;
}

std::shared_ptr<core::InterviewSession> InterviewSessionBuilder::build() {
    return std::make_shared<core::InterviewSession>(id_, user_->id(), mode_, std::move(questions_));
}

} // namespace interview