// OOP: Enumeration Type (Domain Primitive)
#pragma once
#include <string>

namespace core {

enum class Difficulty {
    Easy,
    Medium,
    Hard
};

std::string toString(Difficulty d);
Difficulty fromString(const std::string& s);
int timeLimitMinutes(Difficulty d);
int scoreWeight(Difficulty d);

} // namespace core