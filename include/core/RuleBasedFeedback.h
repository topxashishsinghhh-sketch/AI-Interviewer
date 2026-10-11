// OOP: Strategy Pattern (Concrete Strategy)
#pragma once
#include "core/FeedbackGenerator.h"
#include <map>
#include <vector>

namespace core {

class RuleBasedFeedback : public FeedbackGenerator {
public:
    FeedbackReport generate(const std::vector<Attempt>& attempts,
                            const std::vector<QuestionPtr>& questions) const override;
    
private:
    static std::vector<std::string> generateTips(
        const std::map<std::string, double>& scoreByTopic,
        const std::map<Difficulty, double>& scoreByDifficulty);
};

} // namespace core