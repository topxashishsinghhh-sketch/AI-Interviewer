// OOP: Service Layer (skill profile and gap analysis)
#include "skills/SkillsService.h"
#include "core/Difficulty.h"

#include <algorithm>

namespace skills {

SkillsService::SkillsService(std::shared_ptr<core::IQuestionRepository> questionRepo,
                             std::shared_ptr<core::IAttemptRepository> attemptRepo,
                             std::shared_ptr<core::IUserRepository> userRepo)
    : questionRepo_(std::move(questionRepo))
    , attemptRepo_(std::move(attemptRepo))
    , userRepo_(std::move(userRepo)) {}

SkillProfile SkillsService::profile(const core::UserId& userId) const {
    SkillProfile profile;

    const std::vector<core::QuestionPtr> all = questionRepo_->findAll();
    std::vector<core::Attempt> attempts;
    if (!userId.get().empty()) {
        attempts = attemptRepo_->findByUser(userId);
    }

    // solved / attempted question ids, so each tag counts a user once.
    std::map<std::string, bool> solvedById;
    std::map<std::string, bool> attemptedById;
    for (std::size_t i = 0; i < attempts.size(); ++i) {
        attemptedById[attempts[i].questionId.get()] = true;
        if (attempts[i].correct) solvedById[attempts[i].questionId.get()] = true;
    }

    // Accumulate per tag, split by hard/soft.
    std::map<std::string, SkillRow> rows;
    for (std::size_t i = 0; i < all.size(); ++i) {
        const core::QuestionPtr& q = all[i];
        const std::string id = q->id().get();
        for (std::size_t t = 0; t < q->skillTags().size(); ++t) {
            const std::string tag = q->skillTags()[t];
            SkillRow& row = rows[tag];
            row.tag = tag;
            row.kind = (q->skillKind() == core::SkillKind::Soft) ? "Soft" : "Hard";
            row.available++;
            if (attemptedById.count(id) > 0) row.attempted++;
            if (solvedById.count(id) > 0) row.solved++;
        }
    }

    // Bucket, then compute aggregate mastery.
    int hardAvailable = 0;
    int hardSolved = 0;
    int softAvailable = 0;
    int softSolved = 0;
    for (std::map<std::string, SkillRow>::const_iterator it = rows.begin();
         it != rows.end(); ++it) {
        const SkillRow& row = it->second;
        if (row.kind == "Soft") {
            profile.soft.push_back(row);
            softAvailable += row.available;
            softSolved += row.solved;
        } else {
            profile.hard.push_back(row);
            hardAvailable += row.available;
            hardSolved += row.solved;
        }
    }

    std::sort(profile.hard.begin(), profile.hard.end(),
              [](const SkillRow& a, const SkillRow& b) { return a.tag < b.tag; });
    std::sort(profile.soft.begin(), profile.soft.end(),
              [](const SkillRow& a, const SkillRow& b) { return a.tag < b.tag; });

    profile.hardMastery = hardAvailable > 0
        ? static_cast<double>(hardSolved) / static_cast<double>(hardAvailable) : 0.0;
    profile.softMastery = softAvailable > 0
        ? static_cast<double>(softSolved) / static_cast<double>(softAvailable) : 0.0;

    // Gaps: unsolved tags, weakest first then alphabetical for determinism.
    profile.gaps.reserve(profile.hard.size() + profile.soft.size());
    for (std::size_t i = 0; i < profile.hard.size(); ++i) {
        if (profile.hard[i].solved == 0) profile.gaps.push_back(profile.hard[i]);
    }
    for (std::size_t i = 0; i < profile.soft.size(); ++i) {
        if (profile.soft[i].solved == 0) profile.gaps.push_back(profile.soft[i]);
    }
    std::sort(profile.gaps.begin(), profile.gaps.end(),
              [](const SkillRow& a, const SkillRow& b) {
                  if (a.mastery() != b.mastery()) return a.mastery() < b.mastery();
                  return a.tag < b.tag;
              });

    return profile;
}

std::vector<core::QuestionPtr> SkillsService::practiceFor(const std::string& tag,
                                                          const core::UserId& userId) const {
    std::vector<core::QuestionPtr> all = questionRepo_->findAll();
    std::vector<core::QuestionPtr> matching;
    for (std::size_t i = 0; i < all.size(); ++i) {
        const std::vector<std::string>& tags = all[i]->skillTags();
        if (std::find(tags.begin(), tags.end(), tag) != tags.end()) {
            matching.push_back(all[i]);
        }
    }

    std::vector<core::Attempt> attempts;
    if (!userId.get().empty()) {
        attempts = attemptRepo_->findByUser(userId);
    }
    // Sort by difficulty (Easy first), keeping questions the user has already
    // solved at the end so practice stays useful.
    std::map<std::string, bool> solvedById;
    for (std::size_t i = 0; i < attempts.size(); ++i) {
        if (attempts[i].correct) solvedById[attempts[i].questionId.get()] = true;
    }

    auto difficultyRank = [](core::Difficulty d) {
        switch (d) {
            case core::Difficulty::Easy: return 0;
            case core::Difficulty::Medium: return 1;
            case core::Difficulty::Hard: return 2;
        }
        return 0;
    };

    std::stable_sort(matching.begin(), matching.end(),
        [&difficultyRank, &solvedById](const core::QuestionPtr& a, const core::QuestionPtr& b) {
            const bool aSolved = solvedById.count(a->id().get()) > 0;
            const bool bSolved = solvedById.count(b->id().get()) > 0;
            if (aSolved != bSolved) return !aSolved;
            return difficultyRank(a->difficulty()) < difficultyRank(b->difficulty());
        });

    return matching;
}

} // namespace skills
