// OOP: Abstraction (Abstract Base Class)
#include "core/User.h"
#include "core/StrongId.h"

namespace core {

User::User(UserId id, std::string email, std::string passwordHash,
           std::string name, UserRole role,
           std::string college, std::string branch)
    : id_(std::move(id)), email_(std::move(email)), passwordHash_(std::move(passwordHash)),
      name_(std::move(name)), role_(role), college_(std::move(college)), branch_(std::move(branch)) {}

} // namespace core