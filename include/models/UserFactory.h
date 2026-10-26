#ifndef USER_FACTORY_H
#define USER_FACTORY_H

#include "User.h"
#include <memory>
#include <string>

class UserFactory {
public:
    static std::unique_ptr<User> createUser(
        const std::string& role,
        int userId,
        const std::string& fullName,
        const std::string& email,
        const std::string& passwordHash
    );
};

#endif