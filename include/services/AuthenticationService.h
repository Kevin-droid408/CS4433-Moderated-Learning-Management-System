
#ifndef AUTHENTICATION_SERVICE_H
#define AUTHENTICATION_SERVICE_H

#include "models/User.h"
#include "repositories/UserRepository.h"

#include <memory>
#include <string>

class AuthenticationService {
    private:
        UserRepository& userRepository;

    public:
        explicit AuthenticationService(UserRepository& userRepository);

        std::unique_ptr<User> login(
            const std::string& username,
            const std::string& password
        );
};

#endif
