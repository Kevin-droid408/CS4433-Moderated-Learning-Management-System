#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "repositories/Database.h"
#include "repositories/UserRepository.h"

#include "models/Lecturer.h"
#include "models/HeadOfDepartment.h"

#include "services/PasswordService.h"

#include <iostream>
#include <string>
#include <stdexcept>


/* Reads a password without displaying its characters on Windows. */
string readPassword(const string& prompt) {
    cout << prompt;

#ifdef _WIN32
    HANDLE inputHandle = GetStdHandle(STD_INPUT_HANDLE);
    DWORD originalMode = 0;

    if (inputHandle == INVALID_HANDLE_VALUE ||
        !GetConsoleMode(inputHandle, &originalMode)) {
        throw runtime_error(
            "Secure password input requires a Windows console."
        );
    }

    DWORD hiddenMode = originalMode & ~ENABLE_ECHO_INPUT;

    if (!SetConsoleMode(inputHandle, hiddenMode)) {
        throw runtime_error("Could not enable hidden password input.");
    }

    string password;
    getline(cin, password);

    SetConsoleMode(inputHandle, originalMode);
    cout << '\n';

    if (!cin) {
        throw runtime_error("Could not read the password.");
    }

    return password;
#else
    string password;
    getline(cin, password);
    return password;
#endif
}

/* Creates an account only if its email is not already registered. */
bool createDepartmentAccount(
    UserRepository& repository,
    const string& fullName,
    const string& email,
    const string& role,
    int departmentId) {

    string password = readPassword(
        "Enter password for " + email + ": "
    );

    if (password.empty()) {
        cout << "Password cannot be empty. Account not created.\n";
        return false;
    }

    string confirmation = readPassword(
        "Confirm password: "
    );

    if (password != confirmation) {
        cout << "Passwords do not match. Account not created.\n";
        return false;
    }

    string passwordHash = PasswordService::hashPassword(password);

    if (role == "Lecturer") {
        Lecturer user(0, fullName, email, passwordHash);

        if (!repository.addUser(user, passwordHash, departmentId)) {
            cout << "Account was not created. The email may already exist.\n";
            return false;
        }
    }
    else if (role == "HeadOfDepartment") {
        HeadOfDepartment user(0, fullName, email, passwordHash);

        if (!repository.addUser(user, passwordHash, departmentId)) {
            cout << "Account was not created. The email may already exist.\n";
            return false;
        }
    }
    else {
        cout << "Unsupported account role.\n";
        return false;
    }

    cout << "Account created successfully for " << email << ".\n";
    return true;
}

int main() {
    try {
        Database database("database/lms.db");
        database.initializeSchema("database/schema.sql");

        UserRepository userRepository(database);

        cout << "===== IT DEPARTMENT ACCOUNT SETUP =====\n";
        cout << "Department ID: 2 (Information Technology)\n\n";

        // Confirm that department ID 2 exists before creating accounts.
        database.execute(
            "INSERT OR IGNORE INTO departments "
            "(department_id, department_name) "
            "VALUES (2, 'Information Technology');"
        );

        createDepartmentAccount(
            userRepository,
            "Test IT Lecturer",
            "lecturer.it@example.com",
            "Lecturer",
            2
        );

        cout << '\n';

        createDepartmentAccount(
            userRepository,
            "Test IT Head of Department",
            "hod.it@example.com",
            "HeadOfDepartment",
            2
        );

        cout << "\nSetup finished. Verify the accounts in SQLite.\n";
    }
    catch (const exception& error) {
        cerr << "\nSetup error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
