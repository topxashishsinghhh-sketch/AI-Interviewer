// OOP: State Pattern (Interview State Machine)
#pragma once
#include <string>

namespace core {

enum class InterviewState {
    NotStarted,
    InProgress,
    Paused,
    Finished,
    TimedOut
};

std::string toString(InterviewState s);

} // namespace core