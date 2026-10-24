
#include "repositories/Database.h"
#include "repositories/UserRepository.h"
#include "repositories/CourseRepository.h"
#include "models/User.h"
#include "models/Course.h"
#include "services/AuthenticationService.h"

#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <exception>
#include <vector>

// Reads a menu choice and handles non-numeric input.
int readMenuChoice() {
    int choice;

    while (true) {
        std::cout << "Enter your choice: ";

        if (std::cin >> choice) {
            std::cin.ignore(
                std::numeric_limits<std::streamsize>::max(),
                '\n'
            );
            return choice;
        }

        if (std::cin.eof()) {
            return -1;
        }

        std::cout << "Invalid input. Please enter a number.\n";

        std::cin.clear();
        std::cin.ignore(
            std::numeric_limits<std::streamsize>::max(),
            '\n'
        );
    }
}

// Displays all courses returned by CourseRepository.
void displayAvailableCourses(CourseRepository& courseRepository) {
    const auto courses = courseRepository.getAllCourses();

    std::cout << "\n=== AVAILABLE COURSES ===\n";

    if (courses.empty()) {
        std::cout << "No courses are currently available.\n";
        return;
    }

    for (const auto& course : courses) {
        std::cout << "Course Code: "
                  << course.getCourseCode() << '\n';

        std::cout << "Course Name: "
                  << course.getCourseName() << '\n';

        std::cout << "------------------------------\n";
    }
}

// Displays the menu appropriate to the authenticated user's role.
void showRoleMenu(const User& user) {
    const std::string role = user.getRole();

    std::cout << "\n=== " << role << " MENU ===\n";

    if (role == "Student") {
        std::cout << "1. View available courses\n";
        std::cout << "2. View assignments\n";
        std::cout << "3. Submit assignment\n";
        std::cout << "4. View submission status\n";
        std::cout << "5. View released marks\n";
    }
    else if (role == "Lecturer") {
        std::cout << "1. View my courses\n";
        std::cout << "2. Create assignment\n";
        std::cout << "3. View submissions\n";
        std::cout << "4. Grade submission\n";
    }
    else if (role == "HeadOfDepartment") {
        std::cout << "1. View marks awaiting moderation\n";
        std::cout << "2. Moderate marks\n";
        std::cout << "3. Release marks\n";
    }
    else if (role == "Administrator") {
        std::cout << "1. Manage users\n";
        std::cout << "2. Manage departments\n";
        std::cout << "3. Manage courses\n";
    }
    else {
        std::cout << "Unrecognized user role.\n";
        return;
    }

    std::cout << "0. Log out\n";
}

// Returns the highest menu option for a recognized role.
int getHighestRoleOption(const User& user) {
    const std::string role = user.getRole();

    if (role == "Student") {
        return 5;
    }

    if (role == "Lecturer") {
        return 4;
    }

    if (role == "HeadOfDepartment") {
        return 3;
    }

    if (role == "Administrator") {
        return 3;
    }

    return -1;
}

int main() {
    try {
        // Initialize the database using paths relative to the project root.
        Database database("database/lms.db");
        database.initializeSchema("database/schema.sql");

        // Set up repositories and authentication.
        UserRepository userRepository(database);
        CourseRepository courseRepository(database);

        AuthenticationService authenticationService(userRepository);

        bool running = true;

        while (running) {
            std::cout << "\n==============================\n";
            std::cout << "       MODERATED LMS\n";
            std::cout << "==============================\n";
            std::cout << "1. Login\n";
            std::cout << "2. Exit\n";

            const int choice = readMenuChoice();

            if (choice == 1) {
                std::string email;
                std::string password;

                std::cout << "Email: ";
                if (!std::getline(std::cin, email)) {
                    break;
                }

                std::cout << "Password: ";
                if (!std::getline(std::cin, password)) {
                    break;
                }

                // Authenticate using the existing service.
                std::unique_ptr<User> loggedInUser =
                    authenticationService.login(email, password);

                if (!loggedInUser) {
                    std::cout
                        << "Login failed. Check your credentials "
                        << "and try again.\n";
                    continue;
                }

                std::cout << "\nLogin successful. Welcome, "
                          << loggedInUser->getFullName() << "!\n";

                bool loggedIn = true;

                while (loggedIn && running) {
                    showRoleMenu(*loggedInUser);

                    const int roleChoice = readMenuChoice();

                    if (roleChoice == 0) {
                        loggedIn = false;
                        std::cout << "Logged out successfully.\n";
                    }
                    else if (roleChoice == -1) {
                        loggedIn = false;
                        running = false;
                    }
                    else {
                        const int highestOption =
                            getHighestRoleOption(*loggedInUser);

                        if (roleChoice >= 1 &&
                            roleChoice <= highestOption) {

                            // Implemented Student operation.
                            if (loggedInUser->getRole() == "Student" &&
                                roleChoice == 1) {

                                displayAvailableCourses(courseRepository);
                            }
                            else {
                                std::cout
                                    << "This feature has not been "
                                    << "implemented yet.\n";
                            }
                        }
                        else {
                            std::cout
                                << "Invalid menu option. "
                                << "Please try again.\n";
                        }
                    }
                }

                // The unique_ptr automatically releases the user object
                // when this login scope ends.
            }
            else if (choice == 2) {
                running = false;
                std::cout << "Thank you for using Moderated LMS.\n";
            }
            else if (choice == -1) {
                running = false;
            }
            else {
                std::cout
                    << "Invalid choice. Please select 1 or 2.\n";
            }
        }

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "Application error: "
                  << error.what() << '\n';
        return 1;
    }
}
