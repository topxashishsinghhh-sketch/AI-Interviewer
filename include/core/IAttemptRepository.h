// OOP: Interface Segregation (Repository Contract)
#pragma once
#include "core/Attempt.h"
#include "core/StrongId.h"
#include <vector>
#include <chrono>

namespace core {

class IAttemptRepository {
public:
    virtual ~IAttemptRepository() = default;
    virtual Attempt* findById(const AttemptId& id) = 0; // Returns nullptr if not found
    virtual std::vector<Attempt> findByUser(const UserId& userId) = 0;
    virtual std::vector<Attempt> findByUserAndDateRange(
        const UserId& userId,
        std::chrono::system_clock::time_point from,
        std::chrono::system_clock::time_point to) = 0;
    virtual std::vector<Attempt> findByQuestion(const QuestionId& questionId) = 0;
    virtual void save(const Attempt& attempt) = 0;
    virtual void remove(const AttemptId& id) = 0;
};

} // namespace core