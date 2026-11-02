
#include "services/PasswordService.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

// Tests password hashing and verification.
class PasswordServiceTest : public ::testing::Test {
};

// A password should produce a non-empty hash.
TEST_F(PasswordServiceTest, HashesPasswordSuccessfully) {
    const std::string password = "StudentPassword123";

    const std::string hashedPassword =
        PasswordService::hashPassword(password);

    EXPECT_FALSE(hashedPassword.empty());
    EXPECT_NE(hashedPassword, password);
}

// The correct password should match its stored hash.
TEST_F(PasswordServiceTest, VerifiesCorrectPassword) {
    const std::string password = "StudentPassword123";

    const std::string hashedPassword =
        PasswordService::hashPassword(password);

    EXPECT_TRUE(
        PasswordService::verifyPassword(password, hashedPassword)
    );
}

// An incorrect password should not match.
TEST_F(PasswordServiceTest, RejectsIncorrectPassword) {
    const std::string password = "StudentPassword123";
    const std::string incorrectPassword = "WrongPassword123";

    const std::string hashedPassword =
        PasswordService::hashPassword(password);

    EXPECT_FALSE(
        PasswordService::verifyPassword(
            incorrectPassword,
            hashedPassword
        )
    );
}

// Hashing the same password twice should produce different hashes
// because a new salt is generated each time.
TEST_F(PasswordServiceTest, GeneratesDifferentHashesForSamePassword) {
    const std::string password = "StudentPassword123";

    const std::string firstHash =
        PasswordService::hashPassword(password);

    const std::string secondHash =
        PasswordService::hashPassword(password);

    EXPECT_NE(firstHash, secondHash);

    EXPECT_TRUE(
        PasswordService::verifyPassword(password, firstHash)
    );

    EXPECT_TRUE(
        PasswordService::verifyPassword(password, secondHash)
    );
}

// Empty passwords must not be hashed.
TEST_F(PasswordServiceTest, RejectsEmptyPasswordWhenHashing) {
    EXPECT_THROW(
        PasswordService::hashPassword(""),
        std::invalid_argument
    );
}

// An empty password must fail verification.
TEST_F(PasswordServiceTest, RejectsEmptyPasswordDuringVerification) {
    const std::string hashedPassword =
        PasswordService::hashPassword("StudentPassword123");

    EXPECT_FALSE(
        PasswordService::verifyPassword("", hashedPassword)
    );
}

// An empty stored hash must fail verification.
TEST_F(PasswordServiceTest, RejectsEmptyStoredHash) {
    EXPECT_FALSE(
        PasswordService::verifyPassword(
            "StudentPassword123",
            ""
        )
    );
}

// An invalid stored hash must fail verification.
TEST_F(PasswordServiceTest, RejectsInvalidStoredHash) {
    EXPECT_FALSE(
        PasswordService::verifyPassword(
            "StudentPassword123",
            "invalid-hash"
        )
    );
}
