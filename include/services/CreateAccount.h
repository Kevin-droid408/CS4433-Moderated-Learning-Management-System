
#ifndef CREATE_ACCOUNT_H
#define CREATE_ACCOUNT_H

#include "repositories/UserRepository.h"

#include <string>

class CreateAccount {
    private:
        UserRepository& userRepository;

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
};

#endif
