
#include <gtest/gtest.h>

#include "repositories/Database.h"
#include "repositories/UserRepository.h"
#include "models/UserFactory.h"

#include <memory>
#include <string>
#include <vector>

class UserRepositoryTest : public ::testing::Test {
protected:
    Database database{":memory:"};
    UserRepository userRepository{database};

    void SetUp() override {
        database.execute("PRAGMA foreign_keys = ON;");

        database.execute(
            "CREATE TABLE departments ("
            "department_id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "department_name TEXT NOT NULL UNIQUE"
            ");"
        );

        database.execute(
            "INSERT INTO departments (department_id, department_name) "
            "VALUES (1, 'Computer Science'), "
            "(2, 'Information Technology');"
        );

        database.execute(
            "CREATE TABLE users ("
            "user_id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "username TEXT NOT NULL UNIQUE,"
            "full_name TEXT NOT NULL,"
            "email TEXT NOT NULL UNIQUE,"
            "password_hash TEXT NOT NULL,"
            "role TEXT NOT NULL CHECK (role IN "
            "('Student', 'Lecturer', 'HeadOfDepartment', 'Administrator')),"
            "department_id INTEGER,"
            "FOREIGN KEY (department_id) "
            "REFERENCES departments(department_id)"
            ");"
        );
    }

    std::unique_ptr<User> createStudent(
        const std::string& fullName = "Test Student",
        const std::string& email = "student@example.com"
    ) {
        return UserFactory::createUser(
            "Student",
            0,
            fullName,
            email,
            "test_password_hash"
        );
    }

    bool addTestUser(
        const std::string& username,
        const std::string& email,
        int departmentId = 1
    ) {
        auto user = createStudent("Test Student", email);

        return userRepository.addUser(
            *user,
            username,
            "test_password_hash",
            departmentId
        );
    }
};

// --------------------------------------------------
// ADD USER TESTS
// --------------------------------------------------

TEST_F(UserRepositoryTest, AddUserSuccessfully) {
    auto user = createStudent();

    EXPECT_TRUE(
        userRepository.addUser(
            *user,
            "teststudent",
            "hashed_password",
            1
        )
    );

    EXPECT_TRUE(userRepository.usernameExists("teststudent"));
    EXPECT_TRUE(userRepository.emailExists("student@example.com"));
}

TEST_F(UserRepositoryTest, RejectEmptyUsername) {
    auto user = createStudent();

    EXPECT_FALSE(
        userRepository.addUser(
            *user,
            "",
            "hashed_password",
            1
        )
    );
}

TEST_F(UserRepositoryTest, RejectEmptyPasswordHash) {
    auto user = createStudent();

    EXPECT_FALSE(
        userRepository.addUser(
            *user,
            "teststudent",
            "",
            1
        )
    );
}

TEST_F(UserRepositoryTest, RejectDuplicateUsername) {
    ASSERT_TRUE(addTestUser("student1", "student1@example.com"));

    auto user = createStudent(
        "Another Student",
        "student2@example.com"
    );

    EXPECT_FALSE(
        userRepository.addUser(
            *user,
            "student1",
            "another_hash",
            1
        )
    );
}

TEST_F(UserRepositoryTest, RejectDuplicateEmail) {
    ASSERT_TRUE(addTestUser("student1", "student@example.com"));

    auto user = createStudent(
        "Another Student",
        "student@example.com"
    );

    EXPECT_FALSE(
        userRepository.addUser(
            *user,
            "student2",
            "another_hash",
            1
        )
    );
}

TEST_F(UserRepositoryTest, RejectInvalidDepartment) {
    auto user = createStudent();

    EXPECT_FALSE(
        userRepository.addUser(
            *user,
            "student1",
            "hashed_password",
            999
        )
    );
}

TEST_F(UserRepositoryTest, AddUserWithoutDepartment) {
    auto user = createStudent();

    EXPECT_TRUE(
        userRepository.addUser(
            *user,
            "student1",
            "hashed_password",
            0
        )
    );

    EXPECT_FALSE(userRepository.getDepartmentId(1).has_value());
}

// --------------------------------------------------
// COMPATIBILITY OVERLOAD TESTS
// --------------------------------------------------

TEST_F(UserRepositoryTest, CompatibilityOverloadUsesEmailPrefix) {
    auto user = createStudent(
        "Test Student",
        "student@example.com"
    );

    EXPECT_TRUE(
        userRepository.addUser(
            *user,
            "hashed_password",
            1
        )
    );

    EXPECT_TRUE(userRepository.usernameExists("student"));
}

TEST_F(UserRepositoryTest, CompatibilityOverloadRejectsEmailWithoutAtSign) {
    auto user = createStudent(
        "Test Student",
        "invalidemail"
    );

    EXPECT_FALSE(
        userRepository.addUser(
            *user,
            "hashed_password",
            1
        )
    );
}

TEST_F(UserRepositoryTest, CompatibilityOverloadRejectsEmailStartingWithAtSign) {
    auto user = createStudent(
        "Test Student",
        "@example.com"
    );

    EXPECT_FALSE(
        userRepository.addUser(
            *user,
            "hashed_password",
            1
        )
    );
}

TEST_F(UserRepositoryTest, CompatibilityOverloadRejectsEmptyPasswordHash) {
    auto user = createStudent();

    EXPECT_FALSE(
        userRepository.addUser(
            *user,
            "",
            1
        )
    );
}

// --------------------------------------------------
// FIND USER BY USERNAME TESTS
// --------------------------------------------------

TEST_F(UserRepositoryTest, FindByUsernameSuccessfully) {
    ASSERT_TRUE(
        addTestUser("student1", "student1@example.com")
    );

    auto user = userRepository.findByUsername("student1");

    ASSERT_NE(user, nullptr);

    EXPECT_EQ(user->getFullName(), "Test Student");
    EXPECT_EQ(user->getEmail(), "student1@example.com");
    EXPECT_EQ(user->getUsername(), "student1");
    EXPECT_EQ(user->getRole(), "Student");
}

TEST_F(UserRepositoryTest, FindByUsernameIgnoresCase) {
    ASSERT_TRUE(
        addTestUser("student1", "student1@example.com")
    );

    auto user = userRepository.findByUsername("STUDENT1");

    ASSERT_NE(user, nullptr);
    EXPECT_EQ(user->getUsername(), "student1");
}

TEST_F(UserRepositoryTest, FindByUsernameReturnsNullForUnknownUser) {
    auto user = userRepository.findByUsername("unknown");

    EXPECT_EQ(user, nullptr);
}

TEST_F(UserRepositoryTest, FindByUsernameReturnsNullForEmptyUsername) {
    auto user = userRepository.findByUsername("");

    EXPECT_EQ(user, nullptr);
}

// --------------------------------------------------
// FIND USER BY EMAIL TESTS
// --------------------------------------------------

TEST_F(UserRepositoryTest, FindByEmailSuccessfully) {
    ASSERT_TRUE(
        addTestUser("student1", "student1@example.com")
    );

    auto user = userRepository.findByEmail("student1@example.com");

    ASSERT_NE(user, nullptr);

    EXPECT_EQ(user->getEmail(), "student1@example.com");
    EXPECT_EQ(user->getUsername(), "student1");
}

TEST_F(UserRepositoryTest, FindByEmailReturnsNullForUnknownEmail) {
    auto user = userRepository.findByEmail("unknown@example.com");

    EXPECT_EQ(user, nullptr);
}

// --------------------------------------------------
// USERNAME AND EMAIL EXISTENCE TESTS
// --------------------------------------------------

TEST_F(UserRepositoryTest, UsernameExistsReturnsTrue) {
    ASSERT_TRUE(
        addTestUser("student1", "student1@example.com")
    );

    EXPECT_TRUE(userRepository.usernameExists("student1"));
}

TEST_F(UserRepositoryTest, UsernameExistsIgnoresCase) {
    ASSERT_TRUE(
        addTestUser("student1", "student1@example.com")
    );

    EXPECT_TRUE(userRepository.usernameExists("STUDENT1"));
}

TEST_F(UserRepositoryTest, UsernameExistsReturnsFalseForUnknownUsername) {
    EXPECT_FALSE(userRepository.usernameExists("unknown"));
}

TEST_F(UserRepositoryTest, EmailExistsReturnsTrue) {
    ASSERT_TRUE(
        addTestUser("student1", "student1@example.com")
    );

    EXPECT_TRUE(
        userRepository.emailExists("student1@example.com")
    );
}

TEST_F(UserRepositoryTest, EmailExistsReturnsFalseForUnknownEmail) {
    EXPECT_FALSE(
        userRepository.emailExists("unknown@example.com")
    );
}

// --------------------------------------------------
// DEPARTMENT ID TESTS
// --------------------------------------------------

TEST_F(UserRepositoryTest, GetDepartmentIdSuccessfully) {
    ASSERT_TRUE(
        addTestUser("student1", "student1@example.com", 1)
    );

    std::optional<int> departmentId =
        userRepository.getDepartmentId(1);

    ASSERT_TRUE(departmentId.has_value());
    EXPECT_EQ(departmentId.value(), 1);
}

TEST_F(UserRepositoryTest, GetDepartmentIdReturnsNullForUserWithoutDepartment) {
    auto user = createStudent();

    ASSERT_TRUE(
        userRepository.addUser(
            *user,
            "student1",
            "hashed_password",
            0
        )
    );

    EXPECT_FALSE(
        userRepository.getDepartmentId(1).has_value()
    );
}

TEST_F(UserRepositoryTest, GetDepartmentIdReturnsNullForUnknownUser) {
    EXPECT_FALSE(
        userRepository.getDepartmentId(999).has_value()
    );
}

// --------------------------------------------------
// GET ALL USERS TESTS
// --------------------------------------------------

TEST_F(UserRepositoryTest, GetAllUsersReturnsEmptyList) {
    auto users = userRepository.getAllUsers();

    EXPECT_TRUE(users.empty());
}

TEST_F(UserRepositoryTest, GetAllUsersReturnsAllUsers) {
    ASSERT_TRUE(
        addTestUser("student1", "student1@example.com")
    );

    ASSERT_TRUE(
        addTestUser("student2", "student2@example.com")
    );

    ASSERT_TRUE(
        addTestUser("student3", "student3@example.com")
    );

    auto users = userRepository.getAllUsers();

    ASSERT_EQ(users.size(), 3);

    EXPECT_EQ(users[0]->getUsername(), "student1");
    EXPECT_EQ(users[1]->getUsername(), "student2");
    EXPECT_EQ(users[2]->getUsername(), "student3");
}

TEST_F(UserRepositoryTest, GetAllUsersReturnsUsersInUserIdOrder) {
    ASSERT_TRUE(
        addTestUser("student1", "student1@example.com")
    );

    ASSERT_TRUE(
        addTestUser("student2", "student2@example.com")
    );

    auto users = userRepository.getAllUsers();

    ASSERT_EQ(users.size(), 2);

    EXPECT_EQ(users[0]->getUsername(), "student1");
    EXPECT_EQ(users[1]->getUsername(), "student2");
}
