
#ifndef USER_FACTORY_H
#define USER_FACTORY_H

#include "User.h"

#include <memory>
#include <string>

class UserFactory {
    public:
        static unique_ptr<User> createUser(
            const string& role,
            int userId,
            const string& fullName,
            const string& email,
            const string& passwordHash);
};

#endif
