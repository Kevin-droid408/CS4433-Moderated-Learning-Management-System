
#include "services/AuthenticationService.h"
#include "services/PasswordService.h"

#include <utility>

AuthenticationService::AuthenticationService(
    UserRepository& userRepository
)
    : userRepository(userRepository) {
}

std::unique_ptr<User> AuthenticationService::login(
    const std::string& email,
    const std::string& password
) {
    // Reject empty input.
    if (email.empty() || password.empty()) {
        return nullptr;
    }

    // Find the account using its email address.
    std::unique_ptr<User> user =
        userRepository.findByEmail(email);

    // Use the same failure result for an unknown email
    // and an incorrect password.
    if (!user) {
        return nullptr;
    }

    // Verify the password against the stored hash.
    if (!PasswordService::verifyPassword(
            password,
            user->getPasswordHash())) {
        return nullptr;
    }

    // Authentication succeeded.
    return user;
}
