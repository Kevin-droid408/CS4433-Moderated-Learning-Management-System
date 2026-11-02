#include <gtest/gtest.h>

#include "repositories/Database.h"
#include "repositories/UserRepository.h"

#include "services/AuthenticationService.h"
#include "services/PasswordService.h"

#include "models/Student.h"
#include "models/User.h"

#include <memory>
#include <string>

class AuthenticationServiceTest : public ::testing::Test {
protected:

    // Create an isolated in-memory database.
    Database database{":memory:"};

    // Repository uses the test database.
    UserRepository userRepository{database};

    // Authentication service uses the test repository.
    AuthenticationService authenticationService{userRepository};

    const std::string username = "teststudent";
    const std::string fullName = "Test Student";
    const std::string email = "teststudent@example.com";

    const std::string correctPassword = "StudentPass123!";
    const std::string incorrectPassword = "WrongPassword123!";

    std::string storedPasswordHash;

    void SetUp() override {

        // Create the users table required by UserRepository.
        database.execute(
            "CREATE TABLE users ("
            "user_id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "username TEXT NOT NULL UNIQUE,"
            "full_name TEXT NOT NULL,"
            "email TEXT NOT NULL UNIQUE,"
            "password_hash TEXT NOT NULL,"
            "role TEXT NOT NULL CHECK ("
                "role IN ("
                    "'Student',"
                    "'Lecturer',"
                    "'HeadOfDepartment',"
                    "'Administrator'"
                ")"
            "),"
            "department_id INTEGER"
            ");"
        );

        // Generate a real password hash.
        storedPasswordHash =
            PasswordService::hashPassword(correctPassword);

        // Create a student account.
        Student student(
            1,
            fullName,
            email,
            storedPasswordHash
        );

        // Insert the student into the test database.
        ASSERT_TRUE(
            userRepository.addUser(
                student,
                username,
                storedPasswordHash
            )
        );
    }
};


// 1. Correct username and password should allow login.
TEST_F(
    AuthenticationServiceTest,
    LoginWithCorrectCredentialsReturnsUser
) {
    std::unique_ptr<User> user =
        authenticationService.login(
            username,
            correctPassword
        );

    ASSERT_NE(user, nullptr);

    EXPECT_EQ(user->getUsername(), username);
    EXPECT_EQ(user->getFullName(), fullName);
    EXPECT_EQ(user->getEmail(), email);
    EXPECT_EQ(user->getRole(), "Student");
}


// 2. Incorrect password should prevent login.
TEST_F(
    AuthenticationServiceTest,
    LoginWithIncorrectPasswordReturnsNull
) {
    std::unique_ptr<User> user =
        authenticationService.login(
            username,
            incorrectPassword
        );

    EXPECT_EQ(user, nullptr);
}


// 3. Unknown username should prevent login.
TEST_F(
    AuthenticationServiceTest,
    LoginWithUnknownUsernameReturnsNull
) {
    std::unique_ptr<User> user =
        authenticationService.login(
            "unknownuser",
            correctPassword
        );

    EXPECT_EQ(user, nullptr);
}


// 4. Empty username should prevent login.
TEST_F(
    AuthenticationServiceTest,
    LoginWithEmptyUsernameReturnsNull
) {
    std::unique_ptr<User> user =
        authenticationService.login(
            "",
            correctPassword
        );

    EXPECT_EQ(user, nullptr);
}


// 5. Empty password should prevent login.
TEST_F(
    AuthenticationServiceTest,
    LoginWithEmptyPasswordReturnsNull
) {
    std::unique_ptr<User> user =
        authenticationService.login(
            username,
            ""
        );

    EXPECT_EQ(user, nullptr);
}


// 6. Username lookup should ignore letter case.
TEST_F(
    AuthenticationServiceTest,
    LoginWithDifferentUsernameCaseSucceeds
) {
    std::unique_ptr<User> user =
        authenticationService.login(
            "TESTSTUDENT",
            correctPassword
        );

    ASSERT_NE(user, nullptr);

    EXPECT_EQ(user->getUsername(), username);
    EXPECT_EQ(user->getRole(), "Student");
}