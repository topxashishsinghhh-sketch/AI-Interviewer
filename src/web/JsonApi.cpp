// OOP: Mapper Pattern (domain <-> JSON API payloads)
#include "web/JsonApi.h"
#include "core/Student.h"

namespace web {

json JsonApi::userToJson(const core::UserPtr& user) {
    json out = json::object();
    if (!user) return out;

    out["id"] = user->id().get();
    out["email"] = user->email();
    out["name"] = user->name();
    out["role"] = user->isAdmin() ? "Admin" : "Student";
    out["college"] = user->college();
    out["branch"] = user->branch();

    if (auto student = std::dynamic_pointer_cast<core::Student>(user)) {
        out["streak"] = student->streak();
        out["longestStreak"] = student->longestStreak();
        out["totalSolved"] = student->totalSolved();
        out["totalAttempts"] = student->totalAttempts();
        out["lastActivityDate"] = student->lastActivityDate();
        out["badges"] = student->badges();
    }
    return out;
}

json JsonApi::questionToJson(const core::QuestionPtr& question) {
    json out = json::object();
    if (!question) return out;

    out["id"] = question->id().get();
    out["subject"] = question->subject();
    out["topic"] = question->topic();
    out["difficulty"] = core::toString(question->difficulty());
    out["type"] = question->type();
    out["text"] = question->display();
    return out;
}

json JsonApi::attemptToJson(const core::Attempt& attempt) {
    json out;
    out["questionId"] = attempt.questionId.get();
    out["answer"] = attempt.answer;
    out["correct"] = attempt.correct;
    out["score"] = attempt.score;
    out["difficulty"] = core::toString(attempt.difficulty);
    out["timeTakenSeconds"] = attempt.timeTakenSeconds;
    out["feedback"] = attempt.feedback;
    return out;
}

json JsonApi::reportToJson(const core::FeedbackReport& report) {
    json out;
    out["overallScore"] = report.overallScore;

    json byDifficulty = json::object();
    for (std::map<core::Difficulty, double>::const_iterator it = report.scoreByDifficulty.begin();
         it != report.scoreByDifficulty.end(); ++it) {
        byDifficulty[core::toString(it->first)] = it->second;
    }
    out["scoreByDifficulty"] = byDifficulty;
    out["scoreByTopic"] = report.scoreByTopic;
    out["strongestTopic"] = report.strongestTopic;
    out["weakestTopic"] = report.weakestTopic;
    out["tips"] = report.tips;

    json wrong = json::array();
    for (std::size_t i = 0; i < report.wrongAnswers.size(); ++i) {
        wrong.push_back(attemptToJson(report.wrongAnswers[i]));
    }
    out["wrongAnswers"] = wrong;
    return out;
}

json JsonApi::sessionToJson(const core::InterviewSession& session,
                            const core::QuestionPtr& current) {
    json out;
    out["id"] = session.id().get();
    out["mode"] = session.mode() == core::InterviewSession::Mode::Practice ? "Practice"
                  : session.mode() == core::InterviewSession::Mode::Mock ? "Mock"
                                                                         : "Daily";
    out["state"] = core::toString(session.state());
    out["total"] = session.questions().size();
    out["answered"] = session.attempts().size();
    out["remaining"] = session.questions().size() - session.attempts().size();
    out["finished"] = session.isFinished();

    if (current) {
        out["question"] = questionToJson(current);
    } else {
        out["question"] = nullptr;
    }
    return out;
}

json JsonApi::error(const std::string& message, int status) {
    json out;
    out["error"] = message;
    out["status"] = status;
    return out;
}

bool JsonApi::body(const httplib::Request& request, json& out) {
    try {
        out = json::parse(request.body);
        return out.is_object();
    } catch (const std::exception&) {
        return false;
    }
}

} // namespace web
