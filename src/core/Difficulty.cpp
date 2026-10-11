// OOP: Enumeration Type (Domain Primitive)
#include "core/Difficulty.h"
#include <string>

namespace core {

std::string toString(Difficulty d) {
    switch (d) {
        case Difficulty::Easy: return "Easy";
        case Difficulty::Medium: return "Medium";
        case Difficulty::Hard: return "Hard";
    }
    return "Unknown";
}

Difficulty fromString(const std::string& s) {
    if (s == "Easy") return Difficulty::Easy;
    if (s == "Medium") return Difficulty::Medium;
    if (s == "Hard") return Difficulty::Hard;
    return Difficulty::Easy;
}

int timeLimitMinutes(Difficulty d) {
    switch (d) {
        case Difficulty::Easy: return 1;
        case Difficulty::Medium: return 2;
        case Difficulty::Hard: return 4;
    }
    return 1;
}

int scoreWeight(Difficulty d) {
    switch (d) {
        case Difficulty::Easy: return 1;
        case Difficulty::Medium: return 2;
        case Difficulty::Hard: return 3;
    }
    return 1;
}

} // namespace core