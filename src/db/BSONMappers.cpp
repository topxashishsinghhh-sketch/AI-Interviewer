// OOP: Mapper Pattern (BSON ↔ Domain)
#include "db/BSONMappers.h"
#include "core/Difficulty.h"
#include "core/StrongId.h"

// The stream builder cannot be appended to conditionally (its result is a
// document::value once finalize is called), so all documents here are built
// with the "basic" builder, whose append() accepts key/value pairs and lambdas
// for nested arrays/sub-documents.
#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/basic/sub_array.hpp>
#include <bsoncxx/builder/basic/sub_document.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/oid.hpp>
#include <array>

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::sub_array;
using bsoncxx::builder::basic::sub_document;

namespace db {

namespace {

// ---------------------------------------------------------------------------
// Reading helpers.
//
// document::view::find() and document::element's accessors are non-const in
// this driver version, so documents are scanned with a range-for loop and each
// element is copied out. bsoncxx strings are std::string_view (C++17) and dates
// are an int64 millisecond count.
// ---------------------------------------------------------------------------

bool hasKey(const bsoncxx::document::view& view, const char* key) {
    for (const bsoncxx::document::element& element : view) {
        std::string name(element.key().data(), element.key().size());
        if (name == key) return true;
    }
    return false;
}

std::string stringAt(const bsoncxx::document::view& view, const char* key,
                     const std::string& fallback = std::string()) {
    for (bsoncxx::document::element element : view) {
        std::string name(element.key().data(), element.key().size());
        if (name != key) continue;
        if (element.type() != bsoncxx::type::k_string) return fallback;
        const bsoncxx::stdx::string_view text = element.get_string().value;
        return std::string(text.data(), text.size());
    }
    return fallback;
}

// Reads "_id", falling back to the JSON bank's "id" spelling so a document
// authored as JSON can be mapped without a rewrite stage.
std::string idAt(const bsoncxx::document::view& view) {
    if (hasKey(view, "_id")) return stringAt(view, "_id");
    return stringAt(view, "id");
}

int int32At(const bsoncxx::document::view& view, const char* key, int fallback = 0) {    for (bsoncxx::document::element element : view) {
        std::string name(element.key().data(), element.key().size());
        if (name != key) continue;
        if (element.type() == bsoncxx::type::k_int32) return element.get_int32().value;
        if (element.type() == bsoncxx::type::k_int64) {
            return static_cast<int>(element.get_int64().value);
        }
        return fallback;
    }
    return fallback;
}

double doubleAt(const bsoncxx::document::view& view, const char* key, double fallback = 0.0) {
    for (bsoncxx::document::element element : view) {
        std::string name(element.key().data(), element.key().size());
        if (name != key) continue;
        if (element.type() == bsoncxx::type::k_double) return element.get_double().value;
        if (element.type() == bsoncxx::type::k_int32) {
            return static_cast<double>(element.get_int32().value);
        }
        if (element.type() == bsoncxx::type::k_int64) {
            return static_cast<double>(element.get_int64().value);
        }
        return fallback;
    }
    return fallback;
}

bool boolAt(const bsoncxx::document::view& view, const char* key, bool fallback = false) {
    for (bsoncxx::document::element element : view) {
        std::string name(element.key().data(), element.key().size());
        if (name != key) continue;
        if (element.type() != bsoncxx::type::k_bool) return fallback;
        return element.get_bool().value;
    }
    return fallback;
}

std::chrono::system_clock::time_point dateAt(const bsoncxx::document::view& view, const char* key) {
    const std::chrono::system_clock::time_point empty;
    for (bsoncxx::document::element element : view) {
        std::string name(element.key().data(), element.key().size());
        if (name != key) continue;
        if (element.type() != bsoncxx::type::k_date) return empty;
        return std::chrono::system_clock::time_point(element.get_date().value);
    }
    return empty;
}

// Returns the sub-document stored at key, or an empty view when absent/wrong type.
bsoncxx::document::view documentAt(const bsoncxx::document::view& view, const char* key) {
    static const std::array<unsigned char, 5> emptyDoc = {5, 0, 0, 0, 0};
    for (const bsoncxx::document::element& element : view) {
        std::string name(element.key().data(), element.key().size());
        if (name != key) continue;
        if (element.type() != bsoncxx::type::k_document) {
            return bsoncxx::document::view(emptyDoc.data(), emptyDoc.size());
        }
        return element.get_document().value;
    }
    return bsoncxx::document::view(emptyDoc.data(), emptyDoc.size());
}

bsoncxx::array::view arrayAt(const bsoncxx::document::view& view, const char* key) {
    static const std::array<unsigned char, 5> emptyArray = {5, 0, 0, 0, 0};
    for (const bsoncxx::document::element& element : view) {
        std::string name(element.key().data(), element.key().size());
        if (name != key) continue;
        if (element.type() != bsoncxx::type::k_array) {
            return bsoncxx::array::view(emptyArray.data(), emptyArray.size());
        }
        return element.get_array().value;
    }
    return bsoncxx::array::view(emptyArray.data(), emptyArray.size());
}

std::vector<std::string> stringArrayAt(const bsoncxx::document::view& view, const char* key) {
    std::vector<std::string> result;
    for (const bsoncxx::array::element& item : arrayAt(view, key)) {
        if (item.type() != bsoncxx::type::k_string) continue;
        const bsoncxx::stdx::string_view text = item.get_string().value;
        result.push_back(std::string(text.data(), text.size()));
    }
    return result;
}

std::vector<core::CodingTestCase> testCasesAt(const bsoncxx::document::view& view) {
    std::vector<core::CodingTestCase> result;
    for (const bsoncxx::array::element& item : arrayAt(view, "testCases")) {
        if (item.type() != bsoncxx::type::k_document) continue;
        const bsoncxx::document::view tc = item.get_document().value;
        core::CodingTestCase testCase;
        testCase.input = stringAt(tc, "input");
        testCase.expectedOutput = stringAt(tc, "expectedOutput");
        testCase.isHidden = boolAt(tc, "isHidden");
        result.push_back(testCase);
    }
    return result;
}

bsoncxx::types::b_date toBDate(std::chrono::system_clock::time_point tp) {
    return bsoncxx::types::b_date{tp};
}

} // namespace

// --------------------------------------------------------------------------
// UserMapper
// --------------------------------------------------------------------------

bsoncxx::document::value UserMapper::toBSON(const core::UserPtr& user) {
    using bsoncxx::builder::basic::document;

    document doc;
    doc.append(kvp("_id", user->id().get()),
               kvp("email", user->email()),
               kvp("passwordHash", user->passwordHash()),
               kvp("name", user->name()),
               kvp("role", user->isAdmin() ? "Admin" : "Student"),
               kvp("college", user->college()),
               kvp("branch", user->branch()));

    if (auto student = std::dynamic_pointer_cast<core::Student>(user)) {
        doc.append(kvp("streak", student->streak()),
                   kvp("longestStreak", student->longestStreak()),
                   kvp("totalSolved", student->totalSolved()),
                   kvp("totalAttempts", student->totalAttempts()),
                   kvp("lastActivityDate", student->lastActivityDate()),
                   kvp("badges", [&student](sub_array arr) {
                       for (const std::string& badge : student->badges()) {
                           arr.append(badge);
                       }
                       return arr;
                   }),
                   kvp("bookmarkedQuestions", [&student](sub_array arr) {
                       for (const core::QuestionId& qid : student->bookmarkedQuestions()) {
                           arr.append(qid.get());
                       }
                       return arr;
                   }));
    }

    return doc.extract();
}

core::UserPtr UserMapper::fromBSON(const bsoncxx::document::view& doc) {
    const core::UserId id(idAt(doc));
    const std::string email = stringAt(doc, "email");
    const std::string passwordHash = stringAt(doc, "passwordHash");
    const std::string name = stringAt(doc, "name");
    const std::string roleStr = stringAt(doc, "role", "Student");
    const std::string college = stringAt(doc, "college", "Unspecified");
    const std::string branch = stringAt(doc, "branch", "Unspecified");

    const core::UserRole role = (roleStr == "Admin") ? core::UserRole::Admin
                                                     : core::UserRole::Student;

    if (role == core::UserRole::Admin) {
        return std::make_shared<core::Admin>(id, email, passwordHash, name, college, branch);
    }

    auto student = std::make_shared<core::Student>(id, email, passwordHash, name, college, branch);
    student->setStreak(int32At(doc, "streak"));
    student->setLongestStreak(int32At(doc, "longestStreak"));
    student->setTotalSolved(int32At(doc, "totalSolved"));
    student->setTotalAttempts(int32At(doc, "totalAttempts"));
    if (hasKey(doc, "lastActivityDate")) {
        student->setLastActivityDate(stringAt(doc, "lastActivityDate"));
    }
    for (const std::string& badge : stringArrayAt(doc, "badges")) {
        student->addBadge(badge);
    }
    for (const std::string& qid : stringArrayAt(doc, "bookmarkedQuestions")) {
        student->bookmarkQuestion(core::QuestionId(qid));
    }
    return student;
}

// --------------------------------------------------------------------------
// QuestionMapper
// --------------------------------------------------------------------------

bsoncxx::document::value QuestionMapper::toBSON(const core::QuestionPtr& question) {
    using bsoncxx::builder::basic::document;

    document doc;
    doc.append(kvp("_id", question->id().get()),
               kvp("subject", question->subject()),
               kvp("topic", question->topic()),
               kvp("difficulty", core::toString(question->difficulty())),
               kvp("text", question->text()),
               kvp("explanation", question->explanation()),
               kvp("objective", question->objective()),
               kvp("softSkill", question->skillKind() == core::SkillKind::Soft),
               kvp("skillTags", [&question](sub_array arr) {
                   for (const std::string& tag : question->skillTags()) {
                       arr.append(tag);
                   }
                   return arr;
               }));

    if (auto mcq = std::dynamic_pointer_cast<core::MCQQuestion>(question)) {
        doc.append(kvp("type", "MCQ"),
                   kvp("options", [&mcq](sub_array arr) {
                       for (const std::string& option : mcq->options()) {
                           arr.append(option);
                       }
                       return arr;
                   }),
                   kvp("answer", mcq->correctIndex()));
    } else if (auto text = std::dynamic_pointer_cast<core::TextQuestion>(question)) {
        doc.append(kvp("type", "Text"),
                   kvp("keywords", [&text](sub_array arr) {
                       for (const std::string& keyword : text->keywords()) {
                           arr.append(keyword);
                       }
                       return arr;
                   }),
                   kvp("minKeywords", text->minKeywords()));
    } else if (auto coding = std::dynamic_pointer_cast<core::CodingQuestion>(question)) {
        doc.append(kvp("type", "Coding"),
                   kvp("templateCode", coding->templateCode()),
                   kvp("testCases", [&coding](sub_array arr) {
                       for (const core::CodingTestCase& testCase : coding->testCases()) {
                           arr.append([&testCase](sub_document tc) {
                               tc.append(kvp("input", testCase.input),
                                         kvp("expectedOutput", testCase.expectedOutput),
                                         kvp("isHidden", testCase.isHidden));
                               return tc;
                           });
                       }
                       return arr;
                   }));
    } else {
        doc.append(kvp("type", question->type()));
    }

    return doc.extract();
}

core::QuestionPtr QuestionMapper::fromBSON(const bsoncxx::document::view& doc) {
    const core::QuestionId id(idAt(doc));
    const std::string subject = stringAt(doc, "subject");
    const std::string topic = stringAt(doc, "topic");
    const core::Difficulty difficulty = core::fromString(stringAt(doc, "difficulty", "Easy"));
    const std::string type = stringAt(doc, "type");
    const std::string text = stringAt(doc, "text");
    const std::string explanation = stringAt(doc, "explanation");

    if (type == "MCQ") {
        const std::vector<std::string> options = stringArrayAt(doc, "options");
        const int answer = int32At(doc, "answer");
        auto q = std::make_shared<core::MCQQuestion>(id, subject, topic, difficulty,
                                                     text, options, answer, explanation);
        q->setObjective(stringAt(doc, "objective"));
        q->setSkillKind(has(doc, "softSkill") && boolAt(doc, "softSkill")
                        ? core::SkillKind::Soft : core::SkillKind::Hard);
        q->setSkillTags(stringArrayAt(doc, "skillTags"));
        return q;
    }
    if (type == "Text") {
        const std::vector<std::string> keywords = stringArrayAt(doc, "keywords");
        const int minKeywords = int32At(doc, "minKeywords", 1);
        auto q = std::make_shared<core::TextQuestion>(id, subject, topic, difficulty,
                                                      text, keywords, minKeywords, explanation);
        q->setObjective(stringAt(doc, "objective"));
        q->setSkillKind(has(doc, "softSkill") && boolAt(doc, "softSkill")
                        ? core::SkillKind::Soft : core::SkillKind::Hard);
        q->setSkillTags(stringArrayAt(doc, "skillTags"));
        return q;
    }
    if (type == "Coding") {
        const std::string templateCode = stringAt(doc, "templateCode");
        const std::vector<core::CodingTestCase> testCases = testCasesAt(doc);
        auto q = std::make_shared<core::CodingQuestion>(id, subject, topic, difficulty,
                                                       text, templateCode, testCases, explanation);
        q->setObjective(stringAt(doc, "objective"));
        q->setSkillKind(has(doc, "softSkill") && boolAt(doc, "softSkill")
                        ? core::SkillKind::Soft : core::SkillKind::Hard);
        q->setSkillTags(stringArrayAt(doc, "skillTags"));
        return q;
    }
    return nullptr;
}

// --------------------------------------------------------------------------
// AttemptMapper
// --------------------------------------------------------------------------

bsoncxx::document::value AttemptMapper::toBSON(const core::Attempt& attempt) {
    using bsoncxx::builder::basic::document;

    document doc;
    doc.append(kvp("_id", attempt.id.get()),
               kvp("userId", attempt.userId.get()),
               kvp("questionId", attempt.questionId.get()),
               kvp("answer", attempt.answer),
               kvp("correct", attempt.correct),
               kvp("score", attempt.score),
               kvp("difficulty", core::toString(attempt.difficulty)),
               kvp("timeTakenSeconds", attempt.timeTakenSeconds),
               kvp("timestamp", toBDate(attempt.timestamp)),
               kvp("bookmarked", attempt.bookmarked),
               kvp("feedback", attempt.feedback));
    return doc.extract();
}

core::Attempt AttemptMapper::fromBSON(const bsoncxx::document::view& doc) {
    core::Attempt attempt;
    attempt.id = core::AttemptId(idAt(doc));
    attempt.userId = core::UserId(stringAt(doc, "userId"));
    attempt.questionId = core::QuestionId(stringAt(doc, "questionId"));
    attempt.answer = stringAt(doc, "answer");
    attempt.correct = boolAt(doc, "correct");
    attempt.score = doubleAt(doc, "score");
    attempt.difficulty = core::fromString(stringAt(doc, "difficulty", "Easy"));
    attempt.timeTakenSeconds = int32At(doc, "timeTakenSeconds");
    attempt.timestamp = dateAt(doc, "timestamp");
    attempt.bookmarked = boolAt(doc, "bookmarked");
    attempt.feedback = stringAt(doc, "feedback");
    return attempt;
}

// --------------------------------------------------------------------------
// InterviewSessionMapper
// --------------------------------------------------------------------------

bsoncxx::document::value InterviewSessionMapper::toBSON(const core::InterviewSession& session) {
    using bsoncxx::builder::basic::document;

    document doc;
    doc.append(kvp("_id", session.id().get()),
               kvp("userId", session.userId().get()),
               kvp("mode", session.mode() == core::InterviewSession::Mode::Practice ? "Practice" :
                            session.mode() == core::InterviewSession::Mode::Mock ? "Mock" : "Daily"),
               kvp("state", core::toString(session.state())),
               kvp("startTime", toBDate(session.startTime())),
               kvp("questions", [&session](sub_array arr) {
                   for (const core::QuestionPtr& question : session.questions()) {
                       arr.append(question->id().get());
                   }
                   return arr;
               }));

    const std::vector<core::Attempt>& attempts = session.attempts();
    if (!attempts.empty()) {
        doc.append(kvp("attempts", [&attempts](sub_array arr) {
            for (const core::Attempt& attempt : attempts) {
                arr.append(AttemptMapper::toBSON(attempt));
            }
            return arr;
        }));
    }

    if (session.endTime() != std::chrono::system_clock::time_point{}) {
        doc.append(kvp("endTime", toBDate(session.endTime())));
    }

    return doc.extract();
}

std::shared_ptr<core::InterviewSession> InterviewSessionMapper::fromBSON(
    const bsoncxx::document::view&) {
    // Full reconstruction needs the question and attempt repositories, so the
    // session is rebuilt by the service layer instead of here.
    return nullptr;
}

// --------------------------------------------------------------------------
// FeedbackMapper
// --------------------------------------------------------------------------

bsoncxx::document::value FeedbackMapper::toBSON(
    const core::FeedbackReport& report,
    const core::UserId& userId,
    const core::SessionId& sessionId) {
    using bsoncxx::builder::basic::document;

    document doc;
    doc.append(kvp("_id", bsoncxx::oid{}),
               kvp("userId", userId.get()),
               kvp("sessionId", sessionId.get()),
               kvp("overallScore", report.overallScore),
               kvp("strongestTopic", report.strongestTopic),
               kvp("weakestTopic", report.weakestTopic),
               kvp("timestamp", toBDate(std::chrono::system_clock::now())),
               kvp("scoreByDifficulty", [&report](sub_document sub) {
                   for (const auto& pair : report.scoreByDifficulty) {
                       sub.append(kvp(core::toString(pair.first), pair.second));
                   }
                   return sub;
               }),
               kvp("scoreByTopic", [&report](sub_document sub) {
                   for (const auto& pair : report.scoreByTopic) {
                       sub.append(kvp(pair.first, pair.second));
                   }
                   return sub;
               }),
               kvp("tips", [&report](sub_array arr) {
                   for (const std::string& tip : report.tips) {
                       arr.append(tip);
                   }
                   return arr;
               }),
               kvp("wrongAnswers", [&report](sub_array arr) {
                   for (const core::Attempt& attempt : report.wrongAnswers) {
                       arr.append(AttemptMapper::toBSON(attempt));
                   }
                   return arr;
               }));
    return doc.extract();
}

core::FeedbackReport FeedbackMapper::fromBSON(const bsoncxx::document::view& doc) {
    core::FeedbackReport report;
    report.overallScore = doubleAt(doc, "overallScore");
    report.strongestTopic = stringAt(doc, "strongestTopic");
    report.weakestTopic = stringAt(doc, "weakestTopic");

    const bsoncxx::document::view byDifficulty = documentAt(doc, "scoreByDifficulty");
    for (const bsoncxx::document::element& element : byDifficulty) {
        const std::string key(element.key().data(), element.key().size());
        report.scoreByDifficulty[core::fromString(key)] = doubleAt(byDifficulty, key.c_str());
    }

    const bsoncxx::document::view byTopic = documentAt(doc, "scoreByTopic");
    for (const bsoncxx::document::element& element : byTopic) {
        const std::string key(element.key().data(), element.key().size());
        report.scoreByTopic[key] = doubleAt(byTopic, key.c_str());
    }

    report.tips = stringArrayAt(doc, "tips");
    return report;
}

// --------------------------------------------------------------------------
// ScoreboardMapper
// --------------------------------------------------------------------------

bsoncxx::document::value ScoreboardMapper::toBSON(
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
    int rank) {
    using bsoncxx::builder::basic::document;

    document doc;
    doc.append(kvp("_id", bsoncxx::oid{}),
               kvp("userId", userId),
               kvp("name", name),
               kvp("college", college),
               kvp("branch", branch),
               kvp("score", score),
               kvp("solved", solved),
               kvp("streak", streak),
               kvp("lastActivity", toBDate(lastActivity)),
               kvp("scope", scope),
               kvp("window", window),
               kvp("ranking", ranking),
               kvp("rank", rank));
    return doc.extract();
}

core::ScoreboardEntry ScoreboardMapper::fromBSON(const bsoncxx::document::view& doc) {
    core::ScoreboardEntry entry;
    entry.userId = core::UserId(stringAt(doc, "userId"));
    entry.name = stringAt(doc, "name");
    entry.college = stringAt(doc, "college");
    entry.branch = stringAt(doc, "branch");
    entry.score = doubleAt(doc, "score");
    entry.solved = int32At(doc, "solved");
    entry.streak = int32At(doc, "streak");
    entry.lastActivity = dateAt(doc, "lastActivity");
    entry.rank = int32At(doc, "rank");
    return entry;
}

} // namespace db
