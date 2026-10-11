// OOP: Mapper Pattern (BSON ↔ Domain)
#pragma once
#include "core/User.h"
// Student.h also declares Admin (both live in one header by design)
#include "core/Student.h"
#include "core/Question.h"
#include "core/MCQQuestion.h"
#include "core/TextQuestion.h"
#include "core/CodingQuestion.h"
#include "core/Attempt.h"
#include "core/InterviewSession.h"
#include "core/FeedbackGenerator.h"
#include "core/IScoreboardRepository.h"
#include "core/Difficulty.h"
#include "core/StrongId.h"
#include <bsoncxx/document/view.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/types.hpp>
#include <chrono>
#include <vector>
#include <memory>

namespace db {

class UserMapper {
public:
    static bsoncxx::document::value toBSON(const core::UserPtr& user);
    static core::UserPtr fromBSON(const bsoncxx::document::view& doc);
};

class QuestionMapper {
public:
    static bsoncxx::document::value toBSON(const core::QuestionPtr& question);
    static core::QuestionPtr fromBSON(const bsoncxx::document::view& doc);
};

class AttemptMapper {
public:
    static bsoncxx::document::value toBSON(const core::Attempt& attempt);
    static core::Attempt fromBSON(const bsoncxx::document::view& doc);
};

class InterviewSessionMapper {
public:
    static bsoncxx::document::value toBSON(const core::InterviewSession& session);
    static std::shared_ptr<core::InterviewSession> fromBSON(const bsoncxx::document::view& doc);
};

class FeedbackMapper {
public:
    static bsoncxx::document::value toBSON(
        const core::FeedbackReport& report,
        const core::UserId& userId,
        const core::SessionId& sessionId);
    static core::FeedbackReport fromBSON(const bsoncxx::document::view& doc);
};

class ScoreboardMapper {
public:
    static bsoncxx::document::value toBSON(
        const std::string& userId,
        const std::string& name,
        const std::string& college,
        const std::string& branch,
        double score,
        int solved,
        int streak,
        std::chrono::system_clock::time_point lastActivity,
        const std::string& scope,
        const std::string& window,
        const std::string& ranking,
        int rank);
    static core::ScoreboardEntry fromBSON(const bsoncxx::document::view& doc);
};

} // namespace db