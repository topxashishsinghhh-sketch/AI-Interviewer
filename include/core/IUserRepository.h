// OOP: Interface Segregation (Repository Contract)
#pragma once
#include "core/User.h"
#include "core/StrongId.h"
#include <vector>

namespace core {

class IUserRepository {
public:
    virtual ~IUserRepository() = default;
    virtual UserPtr findById(const UserId& id) = 0; // Returns nullptr if not found
    virtual UserPtr findByEmail(const std::string& email) = 0; // Returns nullptr if not found
    virtual std::vector<UserPtr> findAll() = 0;
    virtual void save(const UserPtr& user) = 0;
    virtual void remove(const UserId& id) = 0;
    virtual bool exists(const std::string& email) = 0;
};

} // namespace core