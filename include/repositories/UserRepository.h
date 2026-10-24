
#ifndef USER_REPOSITORY_H
#define USER_REPOSITORY_H

#include "Database.h"
#include "models/User.h"

#include <memory>
#include <optional>
#include <string>

class UserRepository {
    private:
        Database& database;

    public:
        explicit UserRepository(Database& database);

        bool addUser(
            const User& user,
            const string& passwordHash,
            int departmentId = 0
        );

        unique_ptr<User> findByEmail(
            const string& email
        );
};

#endif
