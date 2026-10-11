// OOP: Repository Pattern (in-memory server-side session state)
#include "web/SessionStore.h"

#include <chrono>
#include <random>
#include <sstream>

namespace web {

SessionStore& SessionStore::instance() {
    static SessionStore store;
    return store;
}

std::string SessionStore::newId(const std::string& prefix) const {
    static std::mt19937_64 generator(
        static_cast<std::uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::ostringstream oss;
    oss << prefix << std::hex << generator();
    return oss.str();
}

std::string SessionStore::createToken(const core::UserPtr& user) {
    if (!user) return std::string();

    std::lock_guard<std::mutex> lock(mutex_);
    const std::string key = user->id().get();

    // Reuse the existing token for this user so a second login does not
    // invalidate an open browser tab.
    std::map<std::string, Entry>::iterator existing = tokensByUser_.find(key);
    if (existing != tokensByUser_.end()) {
        return existing->second.token;
    }

    Entry entry;
    entry.token = newId("tok_");
    entry.user = user;
    const std::string token = entry.token;
    tokensByUser_.insert(std::make_pair(key, entry));
    return token;
}

core::UserPtr SessionStore::userForToken(const std::string& token) const {
    if (token.empty()) return nullptr;

    std::lock_guard<std::mutex> lock(mutex_);
    for (std::map<std::string, Entry>::const_iterator it = tokensByUser_.begin();
         it != tokensByUser_.end(); ++it) {
        if (it->second.token == token) {
            return it->second.user;
        }
    }
    return nullptr;
}

void SessionStore::revokeToken(const std::string& token) {
    if (token.empty()) return;

    std::lock_guard<std::mutex> lock(mutex_);
    for (std::map<std::string, Entry>::iterator it = tokensByUser_.begin();
         it != tokensByUser_.end(); ++it) {
        if (it->second.token == token) {
            tokensByUser_.erase(it);
            return;
        }
    }
}

std::string SessionStore::createSession(const std::shared_ptr<core::InterviewSession>& session,
                                        const core::UserPtr& user) {
    if (!session || !user) return std::string();

    std::lock_guard<std::mutex> lock(mutex_);
    InterviewEntry entry;
    entry.session = session;
    entry.user = user;
    const std::string id = session->id().get().empty() ? newId("iv_") : session->id().get();
    entry.session = session;
    interviews_.insert(std::make_pair(id, entry));
    return id;
}

std::shared_ptr<core::InterviewSession> SessionStore::session(const std::string& id) const {
    if (id.empty()) return nullptr;

    std::lock_guard<std::mutex> lock(mutex_);
    std::map<std::string, InterviewEntry>::const_iterator it = interviews_.find(id);
    if (it == interviews_.end()) return nullptr;
    return it->second.session;
}

core::UserPtr SessionStore::sessionOwner(const std::string& id) const {
    if (id.empty()) return nullptr;

    std::lock_guard<std::mutex> lock(mutex_);
    std::map<std::string, InterviewEntry>::const_iterator it = interviews_.find(id);
    if (it == interviews_.end()) return nullptr;
    return it->second.user;
}

void SessionStore::removeSession(const std::string& id) {
    if (id.empty()) return;

    std::lock_guard<std::mutex> lock(mutex_);
    interviews_.erase(id);
}

} // namespace web
