// OOP: Mapper Pattern (domain <-> JSON API payloads)
#pragma once
#include "core/User.h"
#include "core/Question.h"
#include "core/Attempt.h"
#include "core/InterviewSession.h"
#include "core/FeedbackGenerator.h"
#include "core/Course.h"
// cpp-httplib (header-only) and the vendored nlohmann/json use the same
// "adjacent file" include spelling as the rest of the project.
#include "httplib.h"
#include "json.hpp"

#include <chrono>
#include <string>
#include <vector>

namespace web {

using json = nlohmann::json;

// The only place where domain objects become API payloads and back again. The
// HTTP handlers never build JSON by hand, so the wire format stays in one file.
class JsonApi {
public:
    // ---- outbound ----
    static json userToJson(const core::UserPtr& user);
    static json questionToJson(const core::QuestionPtr& question);
    static json attemptToJson(const core::Attempt& attempt);
    static json reportToJson(const core::FeedbackReport& report);
    static json sessionToJson(const core::InterviewSession& session,
                              const core::QuestionPtr& current);

    // ---- errors ----
    static json error(const std::string& message, int status);

    // ---- inbound ----
    // Reads the JSON body of a request, returning false when it is malformed.
    static bool body(const httplib::Request& request, json& out);
};

} // namespace web
