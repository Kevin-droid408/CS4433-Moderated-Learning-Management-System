#include <gtest/gtest.h>

#include "services/CreateAccount.h"
#include "services/PasswordService.h"
#include "models/UserFactory.h"
#include "repositories/Database.h"
#include "repositories/UserRepository.h"

#include <sqlite3.h>

#include <memory>
#include <string>

class CreateAccountTest : public ::testing::Test {
protected:
    Database database{":memory:"};
    std::unique_ptr<UserRepository> userRepository;
    std::unique_ptr<CreateAccount> createAccount;

    void SetUp() override {
        sqlite3* connection = database.getConnection();

        ASSERT_NE(connection, nullptr);

        // Enable foreign-key constraints for this test database.
        ASSERT_EQ(
            sqlite3_exec(
                connection,
                "PRAGMA foreign_keys = ON;",
                nullptr,
                nullptr,
                nullptr
            ),
            SQLITE_OK
        );

        const char* schema = R"(
            CREATE TABLE departments (
                department_id INTEGER PRIMARY KEY AUTOINCREMENT,
                department_name TEXT NOT NULL UNIQUE
            );

            CREATE TABLE users (
                user_id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT NOT NULL UNIQUE,
                full_name TEXT NOT NULL,
                email TEXT NOT NULL UNIQUE,
                password_hash TEXT NOT NULL,
                role TEXT NOT NULL CHECK (
                    role IN (
                        'Student',
                        'Lecturer',
                        'HeadOfDepartment',
                        'Administrator'
                    )
                ),
                department_id INTEGER,
                FOREIGN KEY (department_id)
                    REFERENCES departments(department_id)
            );

            INSERT INTO departments (department_name)
            VALUES ('Computer Science');

            INSERT INTO departments (department_name)
            VALUES ('Information Technology');
        )";

        ASSERT_EQ(
            sqlite3_exec(
                connection,
                schema,
                nullptr,
                nullptr,
                nullptr
            ),
            SQLITE_OK
        );

        userRepository =
            std::make_unique<UserRepository>(database);

        createAccount =
            std::make_unique<CreateAccount>(*userRepository);
    }

    bool registerStudent(
        const std::string& username = "student1",
        const std::string& fullName = "Test Student",
        const std::string& email = "student1@example.com",
        const std::string& password = "StudentPassword123"
    ) {
        return createAccount->registerAccount(
            username,
            fullName,
            email,
            password,
            "Student"
        );
    }

    std::unique_ptr<User> makeAdministrator() {
        return UserFactory::createUser(
            "Administrator",
            0,
            "Test Administrator",
            "admin@example.com",
            "unused-password-hash"
        );
    }
};

// ---------------------------------------------------------
// Successful account registration
// ---------------------------------------------------------

TEST_F(
    CreateAccountTest,
    RegisterStudentSuccessfully
) {
    EXPECT_TRUE(registerStudent());

    EXPECT_TRUE(
        userRepository->usernameExists("student1")
    );

    EXPECT_TRUE(
        userRepository->emailExists("student1@example.com")
    );
}

TEST_F(
    CreateAccountTest,
    RegisterLecturerWithDepartmentSuccessfully
) {
    EXPECT_TRUE(
        createAccount->registerAccount(
            "lecturer1",
            "Test Lecturer",
            "lecturer1@example.com",
            "LecturerPassword123",
            "Lecturer",
            1
        )
    );

    EXPECT_TRUE(
        userRepository->usernameExists("lecturer1")
    );
}

TEST_F(
    CreateAccountTest,
    UsernameIsNormalizedToLowercase
) {
    ASSERT_TRUE(
        registerStudent(
            "Student.One",
            "Test Student",
            "student.one@example.com"
        )
    );

    // Verify that the username was stored in lowercase.
    auto user = userRepository->findByUsername("student.one");

    ASSERT_NE(user, nullptr);

    EXPECT_EQ(user->getUsername(), "student.one");
}
// ---------------------------------------------------------
// Required-field validation
// ---------------------------------------------------------

TEST_F(
    CreateAccountTest,
    RejectEmptyUsername
) {
    EXPECT_FALSE(
        registerStudent(
            "",
            "Test Student",
            "student@example.com"
        )
    );
}

TEST_F(
    CreateAccountTest,
    RejectEmptyFullName
) {
    EXPECT_FALSE(
        registerStudent(
            "student1",
            "",
            "student@example.com"
        )
    );
}

TEST_F(
    CreateAccountTest,
    RejectEmptyEmail
) {
    EXPECT_FALSE(
        registerStudent(
            "student1",
            "Test Student",
            ""
        )
    );
}

TEST_F(
    CreateAccountTest,
    RejectEmptyPassword
) {
    EXPECT_FALSE(
        registerStudent(
            "student1",
            "Test Student",
            "student@example.com",
            ""
        )
    );
}

// ---------------------------------------------------------
// Username validation and boundary values
// ---------------------------------------------------------

TEST_F(
    CreateAccountTest,
    RejectUsernameShorterThanThreeCharacters
) {
    EXPECT_FALSE(
        registerStudent("ab")
    );
}

TEST_F(
    CreateAccountTest,
    AcceptUsernameWithExactlyThreeCharacters
) {
    EXPECT_TRUE(
        registerStudent(
            "abc",
            "Test Student",
            "abc@example.com"
        )
    );
}

TEST_F(
    CreateAccountTest,
    AcceptUsernameWithExactlyThirtyCharacters
) {
    const std::string username(30, 'a');

    EXPECT_TRUE(
        registerStudent(
            username,
            "Test Student",
            "longusername@example.com"
        )
    );
}

TEST_F(
    CreateAccountTest,
    RejectUsernameLongerThanThirtyCharacters
) {
    const std::string username(31, 'a');

    EXPECT_FALSE(
        registerStudent(
            username,
            "Test Student",
            "longusername@example.com"
        )
    );
}

TEST_F(
    CreateAccountTest,
    RejectUsernameContainingSpaces
) {
    EXPECT_FALSE(
        registerStudent(
            "student one",
            "Test Student",
            "student@example.com"
        )
    );
}

TEST_F(
    CreateAccountTest,
    RejectUsernameContainingHyphen
) {
    EXPECT_FALSE(
        registerStudent(
            "student-one",
            "Test Student",
            "student@example.com"
        )
    );
}

TEST_F(
    CreateAccountTest,
    AcceptUsernameContainingUnderscoreAndPeriod
) {
    EXPECT_TRUE(
        registerStudent(
            "student_one.1",
            "Test Student",
            "student@example.com"
        )
    );
}

// ---------------------------------------------------------
// Duplicate-account validation
// ---------------------------------------------------------

TEST_F(
    CreateAccountTest,
    RejectDuplicateUsername
) {
    ASSERT_TRUE(registerStudent());

    EXPECT_FALSE(
        createAccount->registerAccount(
            "student1",
            "Another Student",
            "another@example.com",
            "AnotherPassword123",
            "Student"
        )
    );
}

TEST_F(
    CreateAccountTest,
    RejectDuplicateEmail
) {
    ASSERT_TRUE(registerStudent());

    EXPECT_FALSE(
        createAccount->registerAccount(
            "student2",
            "Another Student",
            "student1@example.com",
            "AnotherPassword123",
            "Student"
        )
    );
}

TEST_F(
    CreateAccountTest,
    RejectUsernameDuplicateAfterNormalization
) {
    ASSERT_TRUE(
        registerStudent(
            "student1",
            "Test Student",
            "student1@example.com"
        )
    );

    EXPECT_FALSE(
        createAccount->registerAccount(
            "STUDENT1",
            "Another Student",
            "another@example.com",
            "AnotherPassword123",
            "Student"
        )
    );
}

// ---------------------------------------------------------
// Public registration restrictions
// ---------------------------------------------------------

TEST_F(
    CreateAccountTest,
    RejectPublicAdministratorRegistration
) {
    EXPECT_FALSE(
        createAccount->registerAccount(
            "admin1",
            "Test Administrator",
            "admin1@example.com",
            "AdminPassword123",
            "Administrator"
        )
    );
}

TEST_F(
    CreateAccountTest,
    RejectPublicHeadOfDepartmentRegistration
) {
    EXPECT_FALSE(
        createAccount->registerAccount(
            "hod1",
            "Test Head",
            "hod1@example.com",
            "HeadPassword123",
            "HeadOfDepartment",
            1
        )
    );
}

TEST_F(
    CreateAccountTest,
    RejectUnknownRole
) {
    EXPECT_FALSE(
        createAccount->registerAccount(
            "user1",
            "Test User",
            "user1@example.com",
            "UserPassword123",
            "UnknownRole"
        )
    );
}

// ---------------------------------------------------------
// Department validation
// ---------------------------------------------------------

TEST_F(
    CreateAccountTest,
    RejectLecturerWithoutDepartment
) {
    EXPECT_FALSE(
        createAccount->registerAccount(
            "lecturer1",
            "Test Lecturer",
            "lecturer1@example.com",
            "LecturerPassword123",
            "Lecturer"
        )
    );
}

TEST_F(
    CreateAccountTest,
    RejectHeadOfDepartmentWithoutDepartment
) {
    auto administrator = makeAdministrator();

    ASSERT_NE(administrator, nullptr);

    EXPECT_FALSE(
        createAccount->registerAccountByAdministrator(
            *administrator,
            "hod1",
            "Test Head",
            "hod1@example.com",
            "HeadPassword123",
            "HeadOfDepartment"
        )
    );
}

// ---------------------------------------------------------
// Administrator-created accounts
// ---------------------------------------------------------

TEST_F(
    CreateAccountTest,
    RejectAdministratorCreationByStudent
) {
    auto student = UserFactory::createUser(
        "Student",
        0,
        "Test Student",
        "student@example.com",
        "unused-password-hash"
    );

    ASSERT_NE(student, nullptr);

    EXPECT_FALSE(
        createAccount->registerAccountByAdministrator(
            *student,
            "lecturer1",
            "Test Lecturer",
            "lecturer1@example.com",
            "LecturerPassword123",
            "Lecturer",
            1
        )
    );
}

TEST_F(
    CreateAccountTest,
    AdministratorCanCreateLecturer
) {
    auto administrator = makeAdministrator();

    ASSERT_NE(administrator, nullptr);

    EXPECT_TRUE(
        createAccount->registerAccountByAdministrator(
            *administrator,
            "lecturer1",
            "Test Lecturer",
            "lecturer1@example.com",
            "LecturerPassword123",
            "Lecturer",
            1
        )
    );
}

TEST_F(
    CreateAccountTest,
    AdministratorCanCreateHeadOfDepartment
) {
    auto administrator = makeAdministrator();

    ASSERT_NE(administrator, nullptr);

    EXPECT_TRUE(
        createAccount->registerAccountByAdministrator(
            *administrator,
            "hod1",
            "Test Head",
            "hod1@example.com",
            "HeadPassword123",
            "HeadOfDepartment",
            1
        )
    );
}

TEST_F(
    CreateAccountTest,
    AdministratorCanCreateAnotherAdministrator
) {
    auto administrator = makeAdministrator();

    ASSERT_NE(administrator, nullptr);

    EXPECT_TRUE(
        createAccount->registerAccountByAdministrator(
            *administrator,
            "admin2",
            "Another Administrator",
            "admin2@example.com",
            "AdminPassword123",
            "Administrator"
        )
    );
}

TEST_F(
    CreateAccountTest,
    RejectUnknownRoleForAdministrator
) {
    auto administrator = makeAdministrator();

    ASSERT_NE(administrator, nullptr);

    EXPECT_FALSE(
        createAccount->registerAccountByAdministrator(
            *administrator,
            "user1",
            "Test User",
            "user1@example.com",
            "UserPassword123",
            "UnknownRole"
        )
    );
}

// ---------------------------------------------------------
// Password hashing
// ---------------------------------------------------------

TEST_F(
    CreateAccountTest,
    StorePasswordAsHashRatherThanPlainText
) {
    const std::string password = "StudentPassword123";

    ASSERT_TRUE(
        registerStudent(
            "student1",
            "Test Student",
            "student1@example.com",
            password
        )
    );

    auto user = userRepository->findByUsername("student1");

    ASSERT_NE(user, nullptr);

    // The stored password must not be the original password.
    EXPECT_NE(user->getPasswordHash(), password);
}