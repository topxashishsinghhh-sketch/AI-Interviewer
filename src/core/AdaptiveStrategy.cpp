// OOP: Strategy Pattern (Concrete Strategy)
#include "core/AdaptiveStrategy.h"
#include <random>
#include <algorithm>

namespace core {

Difficulty AdaptiveStrategy::adjustDifficulty(Difficulty current, int consecutiveCorrect, int consecutiveWrong) {
    if (consecutiveCorrect >= 2) {
        if (current == Difficulty::Easy) return Difficulty::Medium;
        if (current == Difficulty::Medium) return Difficulty::Hard;
    }
    if (consecutiveWrong >= 2) {
        if (current == Difficulty::Hard) return Difficulty::Medium;
        if (current == Difficulty::Medium) return Difficulty::Easy;
    }
    return current;
}

std::vector<QuestionPtr> AdaptiveStrategy::filterByDifficulty(
    const std::vector<QuestionPtr>& pool, Difficulty d) {
    std::vector<QuestionPtr> result;
    for (const auto& q : pool) {
        if (q->difficulty() == d) result.push_back(q);
    }
    return result;
}

QuestionPtr AdaptiveStrategy::pickRandom(const std::vector<QuestionPtr>& pool) {
    if (pool.empty()) return nullptr;
    static std::random_device rd;
    static std::mt19937 gen(rd());
    // The cast keeps the distribution range an int (it is also what
    // uniform_int_distribution requires).
    const int lastIndex = static_cast<int>(pool.size()) - 1;
    std::uniform_int_distribution<> dis(0, lastIndex);
    return pool[static_cast<std::size_t>(dis(gen))];
}

std::vector<QuestionPtr> AdaptiveStrategy::selectQuestions(
    const std::vector<QuestionPtr>& pool,
    const std::vector<Attempt>& history) const {
    
    std::vector<QuestionPtr> selected;
    
    // Derive the starting difficulty from the user's recent history so repeated
    // daily challenges get harder/easier instead of always starting on Easy.
    Difficulty currentDifficulty = Difficulty::Easy;
    int streak = 0;
    int streakSign = 0; // +1 = answering correctly, -1 = struggling
    for (std::size_t i = history.size(); i > 0; --i) {
        const Attempt& attempt = history[i - 1];
        const int sign = attempt.correct ? 1 : -1;
        if (streakSign == 0) {
            streakSign = sign;
            streak = 1;
        } else if (sign == streakSign) {
            ++streak;
        } else {
            break;
        }
    }
    if (streakSign > 0 && streak >= 3) currentDifficulty = Difficulty::Medium;
    if (streakSign < 0 && streak >= 3) currentDifficulty = Difficulty::Easy;
    
    for (int i = 0; i < 10; ++i) {
        std::vector<QuestionPtr> available = filterByDifficulty(pool, currentDifficulty);
        // Remove already selected (lambda parameter must not shadow the outer `q`)
        available.erase(
            std::remove_if(available.begin(), available.end(),
                [&selected](const QuestionPtr& candidate) {
                    return std::any_of(selected.begin(), selected.end(),
                        [&candidate](const QuestionPtr& chosen) {
                            return chosen->id() == candidate->id();
                        });
                }),
            available.end()
        );
        
        QuestionPtr q = pickRandom(available);
        if (!q) {
            // Fallback to any difficulty that still has unused questions
            const Difficulty order[] = {Difficulty::Easy, Difficulty::Medium, Difficulty::Hard};
            for (Difficulty d : order) {
                if (d == currentDifficulty) continue;
                std::vector<QuestionPtr> fallback = filterByDifficulty(pool, d);
                fallback.erase(
                    std::remove_if(fallback.begin(), fallback.end(),
                        [&selected](const QuestionPtr& candidate) {
                            return std::any_of(selected.begin(), selected.end(),
                                [&candidate](const QuestionPtr& chosen) {
                                    return chosen->id() == candidate->id();
                                });
                        }),
                    fallback.end()
                );
                q = pickRandom(fallback);
                if (q) {
                    currentDifficulty = d;
                    break;
                }
            }
        }
        
        if (q) {
            selected.push_back(q);
        }
    }
    
    return selected;
}

} // namespace core