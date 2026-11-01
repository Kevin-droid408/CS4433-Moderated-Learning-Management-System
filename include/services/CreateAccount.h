
#ifndef CREATE_ACCOUNT_H
#define CREATE_ACCOUNT_H

#include "repositories/UserRepository.h"
#include "models/User.h"

#include <string>

class CreateAccount {
    private:
        UserRepository& userRepository;

        bool createAccount(
            const std::string& username,
            const std::string& fullName,
            const std::string& email,
            const std::string& password,
            const std::string& role,
            int departmentId,
            bool administratorCreated
        );

    public:
        explicit CreateAccount(UserRepository& userRepository);

        bool registerAccount(
            const std::string& username,
            const std::string& fullName,
            const std::string& email,
            const std::string& password,
            const std::string& role,
            int departmentId = 0
        );

        bool registerAccountByAdministrator(
            const User& administrator,
            const std::string& username,
            const std::string& fullName,
            const std::string& email,
            const std::string& password,
            const std::string& role,
            int departmentId = 0
        );
};

#endif
