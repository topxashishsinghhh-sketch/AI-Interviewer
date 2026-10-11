// OOP: Factory Pattern (Object Creation)
#include "interview/SessionFactory.h"
#include "interview/InterviewService.h"
#include "core/LinearRampStrategy.h"
#include "core/AdaptiveStrategy.h"
#include "core/RuleBasedFeedback.h"

namespace interview {

std::unique_ptr<InterviewService> SessionFactory::createPractice(
    std::shared_ptr<core::IQuestionRepository> questionRepo,
    std::shared_ptr<core::IAttemptRepository> attemptRepo,
    std::shared_ptr<core::FeedbackGenerator> feedbackGen) {
    
    auto strategy = std::make_shared<core::LinearRampStrategy>();
    return std::make_unique<InterviewService>(questionRepo, attemptRepo, feedbackGen, strategy);
}

std::unique_ptr<InterviewService> SessionFactory::createMock(
    std::shared_ptr<core::IQuestionRepository> questionRepo,
    std::shared_ptr<core::IAttemptRepository> attemptRepo,
    std::shared_ptr<core::FeedbackGenerator> feedbackGen) {
    
    auto strategy = std::make_shared<core::LinearRampStrategy>();
    return std::make_unique<InterviewService>(questionRepo, attemptRepo, feedbackGen, strategy);
}

std::unique_ptr<InterviewService> SessionFactory::createDaily(
    std::shared_ptr<core::IQuestionRepository> questionRepo,
    std::shared_ptr<core::IAttemptRepository> attemptRepo,
    std::shared_ptr<core::FeedbackGenerator> feedbackGen) {
    
    auto strategy = std::make_shared<core::AdaptiveStrategy>();
    return std::make_unique<InterviewService>(questionRepo, attemptRepo, feedbackGen, strategy);
}

} // namespace interview