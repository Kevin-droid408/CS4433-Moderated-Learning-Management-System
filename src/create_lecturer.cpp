
#include "repositories/Database.h"
#include "repositories/UserRepository.h"
#include "models/Lecturer.h"
#include "services/PasswordService.h"

#include <iostream>
#include <string>
#include <memory>

int main() {
    try {
        Database database("database/lms.db");
        database.initializeSchema("database/schema.sql");

        UserRepository userRepository(database);

        const std::string email = "lecturer.test@example.com";
        const std::string password = "Lecturer-Test-2026!";

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

        Lecturer lecturer(
            0,
            "Test Lecturer",
            email,
            passwordHash
        );

        // Department ID 1 is the existing Computer Science department.
        bool added = userRepository.addUser(
            lecturer,
            passwordHash,
            1
        );

        if (added) {
            std::cout << "\nLecturer account created successfully.\n";
            std::cout << "Email: " << email << '\n';
            std::cout << "Password: " << password << '\n';
        }
        else {
            std::cout << "Could not create the lecturer account.\n";
        }
    }
    catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
