
#include "services/CreateAccount.h"

#include "models/UserFactory.h"
#include "services/PasswordService.h"

#include <cctype>
#include <memory>
#include <stdexcept>
#include <string>

CreateAccount::CreateAccount(UserRepository& userRepository)
    : userRepository(userRepository) {
}

bool CreateAccount::registerAccount(
    const std::string& username,
    const std::string& fullName,
    const std::string& email,
    const std::string& password,
    const std::string& role,
    int departmentId) {

    // Public registration must not grant privileged roles.
    if (role != "Student" && role != "Lecturer") {
        return false;
    }

    if (username.empty() || fullName.empty() ||
        email.empty() || password.empty()) {
        return false;
    }

    // Normalize usernames to lowercase so username matching
    // is consistent regardless of capitalization.
    std::string normalizedUsername = username;

    for (char& character : normalizedUsername) {
        character = static_cast<char>(
            std::tolower(static_cast<unsigned char>(character))
        );
    }

    // Allow simple usernames containing letters, digits,
    // underscores, and periods.
    for (unsigned char character : normalizedUsername) {
        if (!std::isalnum(character) &&
            character != '_' &&
            character != '.') {
            return false;
        }
    }

    if (normalizedUsername.size() < 3 ||
        normalizedUsername.size() > 30) {
        return false;
    }

    if (userRepository.usernameExists(normalizedUsername) ||
        userRepository.emailExists(email)) {
        return false;
    }

    if (role == "Lecturer" && departmentId <= 0) {
        return false;
    }

    if (role == "Student") {
        departmentId = 0;
    }

    const std::string passwordHash =
        PasswordService::hashPassword(password);

    std::unique_ptr<User> user = UserFactory::createUser(
        role,
        0,
        fullName,
        email,
        passwordHash
    );

    user->setUsername(normalizedUsername);

    return userRepository.addUser(
        *user,
        normalizedUsername,
        passwordHash,
        departmentId
    );
}
