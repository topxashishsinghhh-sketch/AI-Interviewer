// OOP: State Pattern (Interview State Machine)
#include "core/InterviewState.h"
#include <string>

namespace core {

std::string toString(InterviewState s) {
    switch (s) {
        case InterviewState::NotStarted: return "NotStarted";
        case InterviewState::InProgress: return "InProgress";
        case InterviewState::Paused: return "Paused";
        case InterviewState::Finished: return "Finished";
        case InterviewState::TimedOut: return "TimedOut";
    }
    return "Unknown";
}

} // namespace core