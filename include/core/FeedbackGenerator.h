// OOP: Strategy Pattern (Algorithm Family)
#pragma once
#include "core/Attempt.h"
#include "core/Question.h"
#include <vector>
#include <string>
#include <map>

namespace core {

struct FeedbackReport {
    double overallScore;
    std::map<Difficulty, double> scoreByDifficulty;
    std::map<std::string, double> scoreByTopic;
    std::string strongestTopic;
    std::string weakestTopic;
    std::vector<std::string> tips;
    std::vector<Attempt> wrongAnswers;
};

class FeedbackGenerator {
public:
    virtual ~FeedbackGenerator() = default;
    virtual FeedbackReport generate(const std::vector<Attempt>& attempts,
                                    const std::vector<QuestionPtr>& questions) const = 0;
};

} // namespace core