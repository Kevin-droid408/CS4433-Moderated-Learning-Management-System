
#ifndef USER_REPOSITORY_H
#define USER_REPOSITORY_H

#include "repositories/Database.h"
#include "models/User.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

class UserRepository {
    private:
        Database& database;

    public:
        explicit UserRepository(Database& database);

        bool addUser(
            const User& user,
            const std::string& username,
            const std::string& passwordHash,
            int departmentId = 0
        );

        // Compatibility overload for existing setup utilities.
        bool addUser(
            const User& user,
            const std::string& passwordHash,
            int departmentId = 0
        );

        std::unique_ptr<User> findByUsername(
            const std::string& username
        );

        std::unique_ptr<User> findByEmail(
            const std::string& email
        );

        bool usernameExists(const std::string& username);
        bool emailExists(const std::string& email);

        std::optional<int> getDepartmentId(int userId);
        std::vector<std::unique_ptr<User>> getAllUsers();
};

#endif
