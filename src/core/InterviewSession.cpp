// OOP: Entity (Identity + Lifecycle)
#include "core/InterviewSession.h"
#include "core/StrongId.h"
#include <chrono>
#include <algorithm>

namespace core {

InterviewSession::InterviewSession(SessionId id, UserId userId, Mode mode,
                                   std::vector<QuestionPtr> questions)
    : id_(std::move(id)), userId_(std::move(userId)), mode_(mode),
      state_(InterviewState::NotStarted), questions_(std::move(questions)) {}

void InterviewSession::start() {
    if (state_ == InterviewState::NotStarted) {
        state_ = InterviewState::InProgress;
        startTime_ = std::chrono::system_clock::now();
        currentQuestionIndex_ = 0;
    }
}

void InterviewSession::pause() {
    if (state_ == InterviewState::InProgress) {
        state_ = InterviewState::Paused;
    }
}

void InterviewSession::resume() {
    if (state_ == InterviewState::Paused) {
        state_ = InterviewState::InProgress;
    }
}

void InterviewSession::finish() {
    if (state_ == InterviewState::InProgress || state_ == InterviewState::Paused) {
        state_ = InterviewState::Finished;
        endTime_ = std::chrono::system_clock::now();
    }
}

void InterviewSession::timeout() {
    if (state_ == InterviewState::InProgress) {
        state_ = InterviewState::TimedOut;
        endTime_ = std::chrono::system_clock::now();
    }
}

bool InterviewSession::submitAnswer(const std::string& answer, int timeTakenSeconds) {
    if (state_ != InterviewState::InProgress || currentQuestionIndex_ >= questions_.size()) {
        return false;
    }
    
    auto question = questions_[currentQuestionIndex_];
    auto eval = question->evaluate(answer);
    
    Attempt attempt(
        AttemptId::generateId(), userId_, question->id(),
        answer, eval.correct, eval.score, question->difficulty(),
        timeTakenSeconds
    );
    attempt.feedback = eval.feedback;
    attempts_.push_back(attempt);
    
    currentQuestionIndex_++;
    if (currentQuestionIndex_ >= questions_.size()) {
        finish();
    }
    return true;
}

bool InterviewSession::skipQuestion(int timeTakenSeconds) {
    if (state_ != InterviewState::InProgress || currentQuestionIndex_ >= questions_.size()) {
        return false;
    }
    
    auto question = questions_[currentQuestionIndex_];
    Attempt attempt(
        AttemptId::generateId(), userId_, question->id(),
        "", false, 0.0, question->difficulty(), timeTakenSeconds
    );
    attempt.feedback = "Skipped";
    attempts_.push_back(attempt);
    
    currentQuestionIndex_++;
    if (currentQuestionIndex_ >= questions_.size()) {
        finish();
    }
    return true;
}

const QuestionPtr InterviewSession::currentQuestion() const {
    if (currentQuestionIndex_ < questions_.size()) {
        return questions_[currentQuestionIndex_];
    }
    return nullptr;
}

double InterviewSession::totalScore() const {
    double total = 0.0;
    for (const auto& attempt : attempts_) {
        total += attempt.score * scoreWeight(attempt.difficulty);
    }
    return total;
}

int InterviewSession::totalTimeSeconds() const {
    int total = 0;
    for (const auto& attempt : attempts_) {
        total += attempt.timeTakenSeconds;
    }
    return total;
}

bool InterviewSession::isFinished() const {
    return state_ == InterviewState::Finished || state_ == InterviewState::TimedOut;
}

} // namespace core