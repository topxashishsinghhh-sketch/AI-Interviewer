// OOP: Service Layer (skill profile and gap analysis)
#pragma once
#include "core/IQuestionRepository.h"
#include "core/IAttemptRepository.h"
#include "core/IUserRepository.h"
#include "core/Question.h"
#include "core/User.h"
#include "core/StrongId.h"
#include <string>
#include <vector>
#include <map>

namespace skills {

// One tag's progress: how many questions carry it, how many the user solved.
struct SkillRow {
    std::string tag;
    std::string kind;    // "Hard" or "Soft"
    int available = 0;
    int solved = 0;
    int attempted = 0;
    double mastery() const {
        return available > 0 ? static_cast<double>(solved) / static_cast<double>(available) : 0.0;
    }
};

struct SkillProfile {
    std::vector<SkillRow> hard;
    std::vector<SkillRow> soft;
    // Tags with zero mastery, weakest first: what to practise next.
    std::vector<SkillRow> gaps;
    double hardMastery = 0.0;
    double softMastery = 0.0;
};

class SkillsService {
public:
    SkillsService(std::shared_ptr<core::IQuestionRepository> questionRepo,
                  std::shared_ptr<core::IAttemptRepository> attemptRepo,
                  std::shared_ptr<core::IUserRepository> userRepo);

    SkillProfile profile(const core::UserId& userId) const;
    // Questions that train a tag, easiest first, for a focused practice run.
    std::vector<core::QuestionPtr> practiceFor(const std::string& tag, const core::UserId& userId) const;

private:
    std::shared_ptr<core::IQuestionRepository> questionRepo_;
    std::shared_ptr<core::IAttemptRepository> attemptRepo_;
    std::shared_ptr<core::IUserRepository> userRepo_;
};

} // namespace skills
