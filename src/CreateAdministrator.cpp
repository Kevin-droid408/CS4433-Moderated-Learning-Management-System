
#include "repositories/Database.h"
#include "repositories/UserRepository.h"
#include "models/Administrator.h"
#include "services/PasswordService.h"

#include <iostream>
#include <string>
#include <memory>
#include <exception>
#include <filesystem>

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

int main() {
    try {
        const std::filesystem::path projectRoot = PROJECT_ROOT;
        const std::filesystem::path databaseDirectory =
            projectRoot / "database";

        std::filesystem::create_directories(databaseDirectory);

        const std::filesystem::path databasePath =
            databaseDirectory / "lms.db";

        const std::filesystem::path schemaPath =
            databaseDirectory / "schema.sql";

        Database database(databasePath.string());
        database.initializeSchema(schemaPath.string());

        UserRepository userRepository(database);

        const std::string username = "admin";
        const std::string fullName = "System Administrator";
        const std::string email = "admin@example.com";

        if (userRepository.findByUsername(username)) {
            std::cout << "An account with username 'admin' already exists.\n";
            std::cout << "No account was created.\n";
            return 0;
        }

        if (userRepository.findByEmail(email)) {
            std::cout << "An account with this email already exists.\n";
            std::cout << "No account was created.\n";
            return 0;
        }

        std::string password;
        std::string confirmation;

        std::cout << "========== INITIAL ADMINISTRATOR SETUP ==========\n";
        std::cout << "Username: " << username << '\n';
        std::cout << "Email: " << email << '\n';
        std::cout << "Enter a strong password: ";

        if (!std::getline(std::cin, password) || password.empty()) {
            std::cout << "\nPassword cannot be empty. Setup cancelled.\n";
            return 1;
        }

        std::cout << "Confirm password: ";

        if (!std::getline(std::cin, confirmation) ||
            password != confirmation) {
            std::cout << "\nPasswords do not match. Setup cancelled.\n";
            return 1;
        }

        const std::string passwordHash =
            PasswordService::hashPassword(password);

        Administrator administrator(
            0, fullName, email, passwordHash
        );

        const bool added = userRepository.addUser(
            administrator,
            username,
            passwordHash,
            0
        );

        if (added) {
            std::cout << "\nAdministrator account created successfully.\n";
            std::cout << "Log in to ModeratedLMS.exe with username: "
                      << username << '\n';
            std::cout << "Use the password you entered during setup.\n";
        } else {
            std::cout << "\nAdministrator account creation failed.\n";
            return 1;
        }
    }
    catch (const std::exception& error) {
        std::cerr << "\nSetup error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
