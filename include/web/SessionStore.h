// OOP: Repository Pattern (in-memory server-side session state)
#pragma once
#include "core/User.h"
#include "core/InterviewSession.h"

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace web {

// Holds the state that HTTP itself does not carry: authenticated browser tokens
// and interviews in progress. Everything here lives in memory for the lifetime
// of the server process, which is all the stateless HTTP protocol needs.
class SessionStore {
public:
    static SessionStore& instance();

    // ---- browser tokens ----
    std::string createToken(const core::UserPtr& user);
    core::UserPtr userForToken(const std::string& token) const;
    void revokeToken(const std::string& token);

    // ---- interviews in progress ----
    std::string createSession(const std::shared_ptr<core::InterviewSession>& session,
                              const core::UserPtr& user);
    std::shared_ptr<core::InterviewSession> session(const std::string& id) const;
    core::UserPtr sessionOwner(const std::string& id) const;
    void removeSession(const std::string& id);

private:
    SessionStore() = default;

    struct Entry {
        std::string token;
        core::UserPtr user;
    };

    struct InterviewEntry {
        std::shared_ptr<core::InterviewSession> session;
        core::UserPtr user;
    };

    // One lock guards both maps; the critical sections are tiny (map lookups
    // and insertion), so a single mutex keeps ordering obvious.
    mutable std::mutex mutex_;
    std::map<std::string, Entry> tokensByUser_;      // key: user id
    std::map<std::string, InterviewEntry> interviews_; // key: session id

    std::string newId(const std::string& prefix) const;
};

} // namespace web
