// OOP: Strategy Pattern (Concrete Strategy)
#include "core/LinearRampStrategy.h"
#include <random>
#include <algorithm>
namespace core {

std::vector<QuestionPtr> LinearRampStrategy::filterByDifficulty(
    const std::vector<QuestionPtr>& pool, Difficulty d) {
    std::vector<QuestionPtr> result;
    for (const auto& q : pool) {
        if (q->difficulty() == d) result.push_back(q);
    }
    return result;
}

QuestionPtr LinearRampStrategy::pickRandom(const std::vector<QuestionPtr>& pool) {
    if (pool.empty()) return nullptr;
    static std::random_device rd;
    static std::mt19937 gen(rd());
    // The cast keeps the distribution range an int (it is also what
    // uniform_int_distribution requires).
    const int lastIndex = static_cast<int>(pool.size()) - 1;
    std::uniform_int_distribution<> dis(0, lastIndex);
    return pool[static_cast<std::size_t>(dis(gen))];
}

std::vector<QuestionPtr> LinearRampStrategy::selectQuestions(
    const std::vector<QuestionPtr>& pool,
    const std::vector<Attempt>& history) const {
    
    std::vector<QuestionPtr> selected;
    auto easy = filterByDifficulty(pool, Difficulty::Easy);
    auto medium = filterByDifficulty(pool, Difficulty::Medium);
    auto hard = filterByDifficulty(pool, Difficulty::Hard);
    
    // A linear ramp is history-independent by design, but skip questions the
    // user has already answered correctly so practice stays fresh.
    for (std::size_t i = 0; i < history.size(); ++i) {
        const Attempt& attempt = history[i];
        if (!attempt.correct) continue;
        const QuestionId& answeredId = attempt.questionId;
        easy.erase(std::remove_if(easy.begin(), easy.end(),
            [&answeredId](const QuestionPtr& q) { return q->id() == answeredId; }),
            easy.end());
        medium.erase(std::remove_if(medium.begin(), medium.end(),
            [&answeredId](const QuestionPtr& q) { return q->id() == answeredId; }),
            medium.end());
        hard.erase(std::remove_if(hard.begin(), hard.end(),
            [&answeredId](const QuestionPtr& q) { return q->id() == answeredId; }),
            hard.end());
    }
    
    // Q1-3: Easy (3 questions)
    for (int i = 0; i < 3 && !easy.empty(); ++i) {
        auto q = pickRandom(easy);
        if (q) {
            selected.push_back(q);
            easy.erase(std::remove(easy.begin(), easy.end(), q), easy.end());
        }
    }
    
    // Q4-7: Medium (4 questions)
    for (int i = 0; i < 4 && !medium.empty(); ++i) {
        auto q = pickRandom(medium);
        if (q) {
            selected.push_back(q);
            medium.erase(std::remove(medium.begin(), medium.end(), q), medium.end());
        }
    }
    
    // Q8-10: Hard (3 questions)
    for (int i = 0; i < 3 && !hard.empty(); ++i) {
        auto q = pickRandom(hard);
        if (q) {
            selected.push_back(q);
            hard.erase(std::remove(hard.begin(), hard.end(), q), hard.end());
        }
    }
    
    return selected;
}

} // namespace core