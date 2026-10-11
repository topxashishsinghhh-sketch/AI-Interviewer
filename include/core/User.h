// OOP: Abstraction (Abstract Base Class)
#pragma once
#include "core/StrongId.h"
#include <string>
#include <memory>
#include <chrono>

namespace core {

enum class UserRole {
    Student,
    Admin
};

class User {
public:
    User(UserId id, std::string email, std::string passwordHash,
         std::string name, UserRole role,
         std::string college = "Unspecified",
         std::string branch = "Unspecified");
    virtual ~User() = default;
    
    const UserId& id() const { return id_; }
    const std::string& email() const { return email_; }
    const std::string& passwordHash() const { return passwordHash_; }
    const std::string& name() const { return name_; }
    UserRole role() const { return role_; }
    const std::string& college() const { return college_; }
    const std::string& branch() const { return branch_; }
    
    void setPasswordHash(const std::string& hash) { passwordHash_ = hash; }
    void setCollege(const std::string& college) { college_ = college; }
    void setBranch(const std::string& branch) { branch_ = branch; }
    
    virtual bool isAdmin() const { return role_ == UserRole::Admin; }
    
protected:
    UserId id_;
    std::string email_;
    std::string passwordHash_;
    std::string name_;
    UserRole role_;
    std::string college_;
    std::string branch_;
};

using UserPtr = std::shared_ptr<User>;

} // namespace core