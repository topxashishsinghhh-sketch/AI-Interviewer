// OOP: Service Layer (Business Logic Facade)
#include "auth/AuthService.h"
#include "core/Student.h"
#include "core/StrongId.h"
#include "core/SHA256.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <unordered_map>
#include <random>

namespace auth {

AuthService::AuthService(std::shared_ptr<core::IUserRepository> userRepo)
    : userRepo_(std::move(userRepo)) {}

AuthResult AuthService::registerUser(const std::string& email, const std::string& password,
                                     const std::string& name, core::UserRole role,
                                     const std::string& college, const std::string& branch) {
    AuthResult result;
    
    if (userRepo_->exists(email)) {
        result.success = false;
        result.message = "Email already registered";
        return result;
    }
    
    core::UserId id = core::UserId::generateId();
    std::string hash = hashPassword(password);
    
    core::UserPtr user;
    if (role == core::UserRole::Admin) {
        user = std::make_shared<core::Admin>(id, email, hash, name, college, branch);
    } else {
        user = std::make_shared<core::Student>(id, email, hash, name, college, branch);
    }
    
    userRepo_->save(user);
    
    result.success = true;
    result.message = "Registration successful";
    result.user = user;
    return result;
}

AuthResult AuthService::login(const std::string& email, const std::string& password) {
    AuthResult result;
    
    core::UserPtr user = userRepo_->findByEmail(email);
    if (!user) {
        result.success = false;
        result.message = "Invalid credentials";
        return result;
    }
    
    // Check lockout
    auto& attempt = failedAttempts_[email];
    auto now = std::chrono::system_clock::now();
    if (attempt.count >= MAX_FAILED_ATTEMPTS) {
        auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - attempt.lastAttempt).count();
        if (elapsed < LOCKOUT_MINUTES) {
            result.success = false;
            result.message = "Account locked. Try again in " + std::to_string(LOCKOUT_MINUTES - elapsed) + " minutes";
            result.remainingAttempts = 0;
            return result;
        } else {
            attempt.count = 0;
        }
    }
    
    if (!verifyPassword(password, user->passwordHash())) {
        attempt.count++;
        attempt.lastAttempt = now;
        result.success = false;
        result.message = "Invalid credentials";
        result.remainingAttempts = MAX_FAILED_ATTEMPTS - attempt.count;
        return result;
    }
    
    // Reset failed attempts on success
    failedAttempts_.erase(email);
    
    result.success = true;
    result.message = "Login successful";
    result.user = user;
    result.remainingAttempts = MAX_FAILED_ATTEMPTS;
    return result;
}

void AuthService::logout(const core::UserId& userId) {
    // No server-side session in console app
    (void)userId;
}

bool AuthService::verifyPassword(const std::string& password, const std::string& hash) const {
    // Format: salt:hash
    size_t pos = hash.find(':');
    if (pos == std::string::npos) return false;
    
    std::string salt = hash.substr(0, pos);
    std::string storedHash = hash.substr(pos + 1);
    
    // Recompute the hash with the stored salt (do NOT generate a new salt)
    std::string computedHash = core::SHA256::hashHex(password + salt);
    
    return computedHash == storedHash;
}

std::string AuthService::hashPassword(const std::string& password) const {
    // Generate salt
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 255);
    
    std::stringstream saltStream;
    for (int i = 0; i < 16; ++i) {
        saltStream << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
    }
    std::string saltStr = saltStream.str();
    
    // Hash password + salt
    std::string saltedPassword = password + saltStr;
    std::string hashHex = core::SHA256::hashHex(saltedPassword);
    
    return saltStr + ":" + hashHex;
}

} // namespace auth