// OOP: Service Layer (Business Logic Facade)
#pragma once
#include "core/InterviewSession.h"
#include "core/QuestionSelectionStrategy.h"
#include "core/IQuestionRepository.h"
#include "core/IAttemptRepository.h"
#include "core/FeedbackGenerator.h"
#include "core/User.h"
#include <memory>
#include <vector>
#include <functional>

namespace interview {

class InterviewService {
public:
    using SessionCallback = std::function<void(const core::InterviewSession&)>;
    
    InterviewService(
        std::shared_ptr<core::IQuestionRepository> questionRepo,
        std::shared_ptr<core::IAttemptRepository> attemptRepo,
        std::shared_ptr<core::FeedbackGenerator> feedbackGen,
        std::shared_ptr<core::QuestionSelectionStrategy> strategy);
    
    std::shared_ptr<core::InterviewSession> startSession(
        const core::UserPtr& user,
        core::InterviewSession::Mode mode,
        const std::string& subject,
        const std::string& topic = "");
    
    bool submitAnswer(std::shared_ptr<core::InterviewSession> session,
                      const std::string& answer, int timeTakenSeconds);
    
    bool skipQuestion(std::shared_ptr<core::InterviewSession> session,
                      int timeTakenSeconds);
    
    core::FeedbackReport finishSession(
        std::shared_ptr<core::InterviewSession> session);
    
    void setStrategy(std::shared_ptr<core::QuestionSelectionStrategy> strategy);
    void setOnSessionComplete(SessionCallback cb);
    
private:
    std::shared_ptr<core::IQuestionRepository> questionRepo_;
    std::shared_ptr<core::IAttemptRepository> attemptRepo_;
    std::shared_ptr<core::FeedbackGenerator> feedbackGen_;
    std::shared_ptr<core::QuestionSelectionStrategy> strategy_;
    SessionCallback onSessionComplete_;
    
    std::vector<core::QuestionPtr> selectQuestionsForSession(
        const std::string& subject, const std::string& topic,
        const core::UserPtr& user);
};

} // namespace interview