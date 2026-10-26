
#include "services/AuthenticationService.h"
#include "services/PasswordService.h"

#include <memory>
#include <string>

AuthenticationService::AuthenticationService(
    UserRepository& userRepository
)
    : userRepository(userRepository) {
}

std::unique_ptr<User> AuthenticationService::login(
    const std::string& username,
    const std::string& password
) {
    if (username.empty() || password.empty()) {
        return nullptr;
    }

    std::unique_ptr<User> user =
        userRepository.findByUsername(username);

    if (!user) {
        return nullptr;
    }

    if (!PasswordService::verifyPassword(
            password,
            user->getPasswordHash())) {
        return nullptr;
    }

    return user;
}
