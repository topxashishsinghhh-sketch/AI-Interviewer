// OOP: Service Layer (Business Logic Facade)
#pragma once
#include "core/User.h"
#include "core/IUserRepository.h"
#include <string>
#include <memory>
#include <chrono>
#include <unordered_map>

namespace auth {

struct AuthResult {
    bool success;
    std::string message;
    core::UserPtr user;
    int remainingAttempts = 5;
};

class AuthService {
public:
    explicit AuthService(std::shared_ptr<core::IUserRepository> userRepo);
    
    AuthResult registerUser(const std::string& email, const std::string& password,
                            const std::string& name, core::UserRole role,
                            const std::string& college = "Unspecified",
                            const std::string& branch = "Unspecified");
    AuthResult login(const std::string& email, const std::string& password);
    void logout(const core::UserId& userId);
    
    bool verifyPassword(const std::string& password, const std::string& hash) const;
    std::string hashPassword(const std::string& password) const;
    
private:
    std::shared_ptr<core::IUserRepository> userRepo_;
    static constexpr int MAX_FAILED_ATTEMPTS = 5;
    static constexpr int LOCKOUT_MINUTES = 15;
    
    struct LoginAttempt {
        int count = 0;
        std::chrono::system_clock::time_point lastAttempt;
    };
    std::unordered_map<std::string, LoginAttempt> failedAttempts_;
};

} // namespace auth