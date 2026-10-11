// OOP: Strategy Pattern (Concrete Strategy)
#pragma once
#include "core/QuestionSelectionStrategy.h"
#include "core/Difficulty.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <random>

namespace core {

class AdaptiveStrategy : public QuestionSelectionStrategy {
public:
    std::vector<QuestionPtr> selectQuestions(
        const std::vector<QuestionPtr>& pool,
        const std::vector<Attempt>& history) const override;
    
    std::string name() const override { return "Adaptive"; }
    
private:
    static Difficulty adjustDifficulty(Difficulty current, int consecutiveCorrect, int consecutiveWrong);
    static std::vector<QuestionPtr> filterByDifficulty(
        const std::vector<QuestionPtr>& pool, Difficulty d);
    static QuestionPtr pickRandom(const std::vector<QuestionPtr>& pool);
};

} // namespace core