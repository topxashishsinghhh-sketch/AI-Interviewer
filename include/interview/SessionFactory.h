// OOP: Factory Pattern (Object Creation)
#pragma once
#include "core/InterviewSession.h"
#include "core/QuestionSelectionStrategy.h"
#include "core/IQuestionRepository.h"
#include "core/IAttemptRepository.h"
#include "core/FeedbackGenerator.h"
#include "interview/InterviewService.h"
#include <memory>

namespace interview {

class SessionFactory {
public:
    static std::unique_ptr<InterviewService> createPractice(
        std::shared_ptr<core::IQuestionRepository> questionRepo,
        std::shared_ptr<core::IAttemptRepository> attemptRepo,
        std::shared_ptr<core::FeedbackGenerator> feedbackGen);
    
    static std::unique_ptr<InterviewService> createMock(
        std::shared_ptr<core::IQuestionRepository> questionRepo,
        std::shared_ptr<core::IAttemptRepository> attemptRepo,
        std::shared_ptr<core::FeedbackGenerator> feedbackGen);
    
    static std::unique_ptr<InterviewService> createDaily(
        std::shared_ptr<core::IQuestionRepository> questionRepo,
        std::shared_ptr<core::IAttemptRepository> attemptRepo,
        std::shared_ptr<core::FeedbackGenerator> feedbackGen);
};

} // namespace interview