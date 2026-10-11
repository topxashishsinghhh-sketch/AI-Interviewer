// OOP: Entity (Identity + Lifecycle)
#pragma once
#include "core/StrongId.h"
#include "core/InterviewState.h"
#include "core/Difficulty.h"
#include "core/Question.h"
#include "core/Attempt.h"
#include <vector>
#include <chrono>
#include <memory>

namespace core {

class InterviewSession {
public:
    enum class Mode {
        Practice,
        Mock,
        Daily
    };
    
    InterviewSession(SessionId id, UserId userId, Mode mode,
                     std::vector<QuestionPtr> questions);
    
    const SessionId& id() const { return id_; }
    const UserId& userId() const { return userId_; }
    Mode mode() const { return mode_; }
    InterviewState state() const { return state_; }
    const std::vector<QuestionPtr>& questions() const { return questions_; }
    const std::vector<Attempt>& attempts() const { return attempts_; }
    size_t currentQuestionIndex() const { return currentQuestionIndex_; }
    std::chrono::system_clock::time_point startTime() const { return startTime_; }
    std::chrono::system_clock::time_point endTime() const { return endTime_; }
    
    void start();
    void pause();
    void resume();
    void finish();
    void timeout();
    
    bool submitAnswer(const std::string& answer, int timeTakenSeconds);
    bool skipQuestion(int timeTakenSeconds);
    
    const QuestionPtr currentQuestion() const;
    double totalScore() const;
    int totalTimeSeconds() const;
    bool isFinished() const;
    
private:
    SessionId id_;
    UserId userId_;
    Mode mode_;
    InterviewState state_;
    std::vector<QuestionPtr> questions_;
    std::vector<Attempt> attempts_;
    size_t currentQuestionIndex_ = 0;
    std::chrono::system_clock::time_point startTime_;
    std::chrono::system_clock::time_point endTime_;
};

} // namespace core