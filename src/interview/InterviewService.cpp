// OOP: Service Layer (Business Logic Facade)
#include "interview/InterviewService.h"
#include "interview/InterviewSessionBuilder.h"
#include "core/StrongId.h"
#include <algorithm>

namespace interview {

InterviewService::InterviewService(
    std::shared_ptr<core::IQuestionRepository> questionRepo,
    std::shared_ptr<core::IAttemptRepository> attemptRepo,
    std::shared_ptr<core::FeedbackGenerator> feedbackGen,
    std::shared_ptr<core::QuestionSelectionStrategy> strategy)
    : questionRepo_(std::move(questionRepo))
    , attemptRepo_(std::move(attemptRepo))
    , feedbackGen_(std::move(feedbackGen))
    , strategy_(std::move(strategy)) {}

std::shared_ptr<core::InterviewSession> InterviewService::startSession(
    const core::UserPtr& user,
    core::InterviewSession::Mode mode,
    const std::string& subject,
    const std::string& topic) {
    
    auto questions = selectQuestionsForSession(subject, topic, user);
    if (questions.empty()) {
        return nullptr;
    }
    
    auto session = InterviewSessionBuilder()
        .withId(core::SessionId::generateId())
        .withUser(user)
        .withMode(mode)
        .withQuestions(std::move(questions))
        .build();
    
    session->start();
    return session;
}

bool InterviewService::submitAnswer(std::shared_ptr<core::InterviewSession> session,
                                    const std::string& answer, int timeTakenSeconds) {
    if (!session) return false;
    return session->submitAnswer(answer, timeTakenSeconds);
}

bool InterviewService::skipQuestion(std::shared_ptr<core::InterviewSession> session,
                                    int timeTakenSeconds) {
    if (!session) return false;
    return session->skipQuestion(timeTakenSeconds);
}

core::FeedbackReport InterviewService::finishSession(
    std::shared_ptr<core::InterviewSession> session) {
    
    if (!session) {
        return core::FeedbackReport();
    }
    
    if (!session->isFinished()) {
        session->finish();
    }
    
    // Save attempts
    for (const auto& attempt : session->attempts()) {
        attemptRepo_->save(attempt);
    }
    
    // Generate feedback
    auto report = feedbackGen_->generate(session->attempts(), session->questions());
    
    // Notify callback
    if (onSessionComplete_) {
        onSessionComplete_(*session);
    }
    
    return report;
}

void InterviewService::setStrategy(std::shared_ptr<core::QuestionSelectionStrategy> strategy) {
    strategy_ = std::move(strategy);
}

void InterviewService::setOnSessionComplete(SessionCallback cb) {
    onSessionComplete_ = std::move(cb);
}

std::vector<core::QuestionPtr> InterviewService::selectQuestionsForSession(
    const std::string& subject, const std::string& topic,
    const core::UserPtr& user) {
    
    std::vector<core::QuestionPtr> pool;
    if (!topic.empty()) {
        pool = questionRepo_->findByTopic(topic);
    } else {
        pool = questionRepo_->findBySubject(subject);
    }
    
    // Get user history for adaptive strategy
    std::vector<core::Attempt> history;
    if (user) {
        history = attemptRepo_->findByUser(user->id());
    }
    
    return strategy_->selectQuestions(pool, history);
}

} // namespace interview