
#include "repositories/Database.h"
#include "repositories/UserRepository.h"
#include "models/HeadOfDepartment.h"
#include "services/PasswordService.h"

#include <iostream>
#include <string>
#include <memory>
#include <exception>

int main() {
    try {
        Database database("database/lms.db");
        database.initializeSchema("database/schema.sql");

        UserRepository userRepository(database);

        const std::string email = "hod.test@example.com";
        const std::string password = "HOD-Test-2026!";

        // Avoid creating a duplicate account.
        std::unique_ptr<User> existingUser =
            userRepository.findByEmail(email);

        if (existingUser) {
            std::cout << "An account with this email already exists.\n";
            return 0;
        }

        // Hash the password before storing it.
        std::string passwordHash =
            PasswordService::hashPassword(password);

        HeadOfDepartment hod(
            0,
            "Test Head of Department",
            email,
            passwordHash
        );

        // Department ID 1 is Computer Science.
        bool added = userRepository.addUser(
            hod,
            passwordHash,
            1
        );

        if (added) {
            std::cout << "\nHOD account created successfully.\n";
            std::cout << "Email: " << email << '\n';
            std::cout << "Password: " << password << '\n';
        }
        else {
            std::cout << "Could not create the HOD account.\n";
        }
    }
    catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
