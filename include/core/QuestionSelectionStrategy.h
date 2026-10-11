// OOP: Strategy Pattern (Algorithm Family)
#pragma once
#include "core/Question.h"
#include "core/Attempt.h"
#include <vector>
#include <memory>

namespace core {

class QuestionSelectionStrategy {
public:
    virtual ~QuestionSelectionStrategy() = default;
    virtual std::vector<QuestionPtr> selectQuestions(
        const std::vector<QuestionPtr>& pool,
        const std::vector<Attempt>& history) const = 0;
    virtual std::string name() const = 0;
};

} // namespace core