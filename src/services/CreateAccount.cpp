
#include "services/CreateAccount.h"

#include "models/UserFactory.h"
#include "services/PasswordService.h"

#include <cctype>
#include <memory>
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

    // Public registration cannot create privileged accounts.
    if (role != "Student" && role != "Lecturer") {
        return false;
    }

    return createAccount(
        username,
        fullName,
        email,
        password,
        role,
        departmentId,
        false
    );
}

bool CreateAccount::registerAccountByAdministrator(
    const User& administrator,
    const std::string& username,
    const std::string& fullName,
    const std::string& email,
    const std::string& password,
    const std::string& role,
    int departmentId) {

    // Only an authenticated Administrator may use this method.
    if (administrator.getRole() != "Administrator") {
        return false;
    }

    if (role != "Student" &&
        role != "Lecturer" &&
        role != "HeadOfDepartment" &&
        role != "Administrator") {
        return false;
    }

    return createAccount(
        username,
        fullName,
        email,
        password,
        role,
        departmentId,
        true
    );
}

bool CreateAccount::createAccount(
    const std::string& username,
    const std::string& fullName,
    const std::string& email,
    const std::string& password,
    const std::string& role,
    int departmentId,
    bool administratorCreated) {

    if (username.empty() ||
        fullName.empty() ||
        email.empty() ||
        password.empty()) {
        return false;
    }

    if (role != "Student" &&
        role != "Lecturer" &&
        role != "HeadOfDepartment" &&
        role != "Administrator") {
        return false;
    }

    // Privileged roles can only be created by an Administrator.
    if (!administratorCreated &&
        role != "Student" &&
        role != "Lecturer") {
        return false;
    }

    // Normalize the username to lowercase.
    std::string normalizedUsername = username;

    for (char& character : normalizedUsername) {
        character = static_cast<char>(
            std::tolower(
                static_cast<unsigned char>(character)
            )
        );
    }

    // Allow letters, digits, underscores and periods.
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

    // Lecturer and Head of Department accounts need a department.
    if ((role == "Lecturer" || role == "HeadOfDepartment") &&
        departmentId <= 0) {
        return false;
    }

    // Students and Administrators are not assigned a department
    // by this account-creation operation.
    if (role == "Student" || role == "Administrator") {
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
