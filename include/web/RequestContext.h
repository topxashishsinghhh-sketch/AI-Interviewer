// OOP: Value Object (per-request immutable context)
#pragma once
#include <string>

namespace web {

// Everything an endpoint needs about the caller, resolved once by the auth
// middleware so handlers never re-parse tokens.
class RequestContext {
public:
    RequestContext(bool authenticated, const std::string& userId,
                   const std::string& role, const std::string& token)
        : authenticated_(authenticated), userId_(userId), role_(role), token_(token) {}

    bool authenticated() const { return authenticated_; }
    const std::string& userId() const { return userId_; }
    const std::string& role() const { return role_; }
    const std::string& token() const { return token_; }
    bool isAdmin() const { return role_ == "Admin"; }

private:
    bool authenticated_ = false;
    std::string userId_;
    std::string role_;
    std::string token_;
};

} // namespace web
