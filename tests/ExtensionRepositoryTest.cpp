
#include <gtest/gtest.h>
#include <sqlite3.h>

#include <string>

#include "repositories/Database.h"
#include "repositories/ExtensionRepository.h"

class ExtensionRepositoryTest : public ::testing::Test {
protected:
    Database* database = nullptr;
    ExtensionRepository* repository = nullptr;

    void SetUp() override {
        database = new Database(":memory:");

        createTables();
        insertTestData();

        repository = new ExtensionRepository(*database);
    }

    void TearDown() override {
        delete repository;
        repository = nullptr;

        delete database;
        database = nullptr;
    }

    void createTables() {
        database->execute(
            "CREATE TABLE users ("
            "user_id INTEGER PRIMARY KEY,"
            "role TEXT NOT NULL,"
            "department_id INTEGER"
            ");"
        );

        database->execute(
            "CREATE TABLE courses ("
            "course_id INTEGER PRIMARY KEY,"
            "department_id INTEGER NOT NULL"
            ");"
        );

        database->execute(
            "CREATE TABLE assignments ("
            "assignment_id INTEGER PRIMARY KEY,"
            "course_id INTEGER NOT NULL,"
            "due_date TEXT NOT NULL"
            ");"
        );

        database->execute(
            "CREATE TABLE enrollments ("
            "student_id INTEGER NOT NULL,"
            "course_id INTEGER NOT NULL,"
            "PRIMARY KEY (student_id, course_id)"
            ");"
        );

        database->execute(
            "CREATE TABLE extensions ("
            "extension_id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "assignment_id INTEGER NOT NULL,"
            "student_id INTEGER NOT NULL,"
            "new_due_date TEXT NOT NULL,"
            "approved_by INTEGER NOT NULL,"
            "reason TEXT NOT NULL,"
            "UNIQUE (assignment_id, student_id)"
            ");"
        );
    }

    void insertTestData() {
        database->execute(
            "INSERT INTO users "
            "(user_id, role, department_id) VALUES "
            "(10, 'Lecturer', 1),"
            "(11, 'Lecturer', 2),"
            "(20, 'Student', 1),"
            "(21, 'Student', 1),"
            "(30, 'Administrator', 1);"
        );

        database->execute(
            "INSERT INTO courses "
            "(course_id, department_id) VALUES "
            "(100, 1),"
            "(200, 2);"
        );

        database->execute(
            "INSERT INTO assignments "
            "(assignment_id, course_id, due_date) VALUES "
            "(500, 100, '2026-11-10'),"
            "(600, 200, '2026-11-10');"
        );

        database->execute(
            "INSERT INTO enrollments "
            "(student_id, course_id) VALUES "
            "(20, 100),"
            "(21, 100);"
        );
    }

    int getExtensionCount() {
        sqlite3_stmt* statement = nullptr;

        const char* sql =
            "SELECT COUNT(*) FROM extensions;";

        int result = sqlite3_prepare_v2(
            database->getConnection(),
            sql,
            -1,
            &statement,
            nullptr
        );

        if (result != SQLITE_OK) {
            return -1;
        }

        int count = 0;

        if (sqlite3_step(statement) == SQLITE_ROW) {
            count = sqlite3_column_int(statement, 0);
        }

        sqlite3_finalize(statement);

        return count;
    }

    std::string getExtensionValue(const char* column) {
        // Only fixed column names from the tests are passed here.
        std::string sql =
            "SELECT " + std::string(column) +
            " FROM extensions "
            "WHERE assignment_id = 500 AND student_id = 20;";

        sqlite3_stmt* statement = nullptr;

        int result = sqlite3_prepare_v2(
            database->getConnection(),
            sql.c_str(),
            -1,
            &statement,
            nullptr
        );

        if (result != SQLITE_OK) {
            return "";
        }

        std::string value;

        if (sqlite3_step(statement) == SQLITE_ROW) {
            const unsigned char* text =
                sqlite3_column_text(statement, 0);

            if (text != nullptr) {
                value = reinterpret_cast<const char*>(text);
            }
        }

        sqlite3_finalize(statement);

        return value;
    }
};

// 1. Grant an extension successfully.
TEST_F(ExtensionRepositoryTest, GrantIndividualExtensionSuccessfully) {
    bool result = repository->grantIndividualExtension(
        500,
        20,
        "2026-11-15",
        10,
        "Medical circumstances"
    );

    EXPECT_TRUE(result);
    EXPECT_EQ(getExtensionCount(), 1);
}

// 2. Reject an invalid assignment ID.
TEST_F(ExtensionRepositoryTest, RejectInvalidAssignmentId) {
    EXPECT_FALSE(
        repository->grantIndividualExtension(
            0, 20, "2026-11-15", 10, "Valid reason"
        )
    );

    EXPECT_FALSE(
        repository->grantIndividualExtension(
            -1, 20, "2026-11-15", 10, "Valid reason"
        )
    );
}

// 3. Reject an invalid student ID.
TEST_F(ExtensionRepositoryTest, RejectInvalidStudentId) {
    EXPECT_FALSE(
        repository->grantIndividualExtension(
            500, 0, "2026-11-15", 10, "Valid reason"
        )
    );

    EXPECT_FALSE(
        repository->grantIndividualExtension(
            500, -1, "2026-11-15", 10, "Valid reason"
        )
    );
}

// 4. Reject an invalid lecturer ID.
TEST_F(ExtensionRepositoryTest, RejectInvalidLecturerId) {
    EXPECT_FALSE(
        repository->grantIndividualExtension(
            500, 20, "2026-11-15", 0, "Valid reason"
        )
    );

    EXPECT_FALSE(
        repository->grantIndividualExtension(
            500, 20, "2026-11-15", -1, "Valid reason"
        )
    );
}

// 5. Reject a non-existent assignment.
TEST_F(ExtensionRepositoryTest, RejectNonExistentAssignment) {
    EXPECT_FALSE(
        repository->grantIndividualExtension(
            999, 20, "2026-11-15", 10, "Valid reason"
        )
    );
}

// 6. Reject a non-existent student.
TEST_F(ExtensionRepositoryTest, RejectNonExistentStudent) {
    EXPECT_FALSE(
        repository->grantIndividualExtension(
            500, 999, "2026-11-15", 10, "Valid reason"
        )
    );
}

// 7. Reject a non-existent lecturer.
TEST_F(ExtensionRepositoryTest, RejectNonExistentLecturer) {
    EXPECT_FALSE(
        repository->grantIndividualExtension(
            500, 20, "2026-11-15", 999, "Valid reason"
        )
    );
}

// 8. Reject a user who is not a lecturer.
TEST_F(ExtensionRepositoryTest, RejectNonLecturerAccount) {
    EXPECT_FALSE(
        repository->grantIndividualExtension(
            500, 20, "2026-11-15", 30, "Valid reason"
        )
    );
}

// 9. Reject a lecturer from another department.
TEST_F(ExtensionRepositoryTest, RejectLecturerFromDifferentDepartment) {
    EXPECT_FALSE(
        repository->grantIndividualExtension(
            500, 20, "2026-11-15", 11, "Valid reason"
        )
    );
}

// 10. Reject a student who is not enrolled in the course.
TEST_F(ExtensionRepositoryTest, RejectStudentNotEnrolledInCourse) {
    EXPECT_FALSE(
        repository->grantIndividualExtension(
            500, 999, "2026-11-15", 10, "Valid reason"
        )
    );
}

// 11. Reject an invalid date format.
TEST_F(ExtensionRepositoryTest, RejectInvalidDateFormat) {
    EXPECT_FALSE(
        repository->grantIndividualExtension(
            500, 20, "15-11-2026", 10, "Valid reason"
        )
    );

    EXPECT_FALSE(
        repository->grantIndividualExtension(
            500, 20, "2026/11/15", 10, "Valid reason"
        )
    );

    EXPECT_FALSE(
        repository->grantIndividualExtension(
            500, 20, "2026-11", 10, "Valid reason"
        )
    );
}

// 12. Reject impossible calendar dates.
TEST_F(ExtensionRepositoryTest, RejectImpossibleCalendarDates) {
    EXPECT_FALSE(
        repository->grantIndividualExtension(
            500, 20, "2026-02-30", 10, "Valid reason"
        )
    );

    EXPECT_FALSE(
        repository->grantIndividualExtension(
            500, 20, "2026-13-10", 10, "Valid reason"
        )
    );

    EXPECT_FALSE(
        repository->grantIndividualExtension(
            500, 20, "2026-04-31", 10, "Valid reason"
        )
    );
}

// 13. Accept a valid leap-year date when it is later than the deadline.
TEST_F(ExtensionRepositoryTest, AcceptValidLeapYearDate) {
    database->execute(
        "UPDATE assignments "
        "SET due_date = '2024-02-20' "
        "WHERE assignment_id = 500;"
    );

    EXPECT_TRUE(
        repository->grantIndividualExtension(
            500, 20, "2024-02-29", 10, "Valid reason"
        )
    );
}

// 14. Reject an empty reason.
TEST_F(ExtensionRepositoryTest, RejectEmptyReason) {
    EXPECT_FALSE(
        repository->grantIndividualExtension(
            500, 20, "2026-11-15", 10, ""
        )
    );
}

// 15. Reject a whitespace-only reason.
TEST_F(ExtensionRepositoryTest, RejectWhitespaceOnlyReason) {
    EXPECT_FALSE(
        repository->grantIndividualExtension(
            500, 20, "2026-11-15", 10, "   \t\n"
        )
    );
}

// 16. Reject a date earlier than the original deadline.
TEST_F(ExtensionRepositoryTest, RejectEarlierDueDate) {
    EXPECT_FALSE(
        repository->grantIndividualExtension(
            500, 20, "2026-11-09", 10, "Valid reason"
        )
    );
}

// 17. Reject a date equal to the original deadline.
TEST_F(ExtensionRepositoryTest, RejectSameDueDate) {
    EXPECT_FALSE(
        repository->grantIndividualExtension(
            500, 20, "2026-11-10", 10, "Valid reason"
        )
    );
}

// 18. Verify the saved extension details.
TEST_F(ExtensionRepositoryTest, SaveCorrectExtensionDetails) {
    EXPECT_TRUE(
        repository->grantIndividualExtension(
            500,
            20,
            "2026-11-15",
            10,
            "Approved circumstances"
        )
    );

    EXPECT_EQ(
        getExtensionValue("new_due_date"),
        "2026-11-15"
    );

    EXPECT_EQ(
        getExtensionValue("approved_by"),
        "10"
    );

    EXPECT_EQ(
        getExtensionValue("reason"),
        "Approved circumstances"
    );
}

// 19. Update an existing extension instead of inserting a duplicate.
TEST_F(ExtensionRepositoryTest, UpdateExistingExtension) {
    EXPECT_TRUE(
        repository->grantIndividualExtension(
            500, 20, "2026-11-15", 10, "First reason"
        )
    );

    EXPECT_TRUE(
        repository->grantIndividualExtension(
            500, 20, "2026-11-20", 10, "Updated reason"
        )
    );

    EXPECT_EQ(getExtensionCount(), 1);

    EXPECT_EQ(
        getExtensionValue("new_due_date"),
        "2026-11-20"
    );

    EXPECT_EQ(
        getExtensionValue("reason"),
        "Updated reason"
    );
}

// 20. Allow separate extensions for different students.
TEST_F(ExtensionRepositoryTest, AllowExtensionsForDifferentStudents) {
    EXPECT_TRUE(
        repository->grantIndividualExtension(
            500, 20, "2026-11-15", 10, "Reason for student 20"
        )
    );

    EXPECT_TRUE(
        repository->grantIndividualExtension(
            500, 21, "2026-11-18", 10, "Reason for student 21"
        )
    );

    EXPECT_EQ(getExtensionCount(), 2);
}

// 21. Reject an invalid date containing extra characters.
TEST_F(ExtensionRepositoryTest, RejectDateWithExtraCharacters) {
    EXPECT_FALSE(
        repository->grantIndividualExtension(
            500, 20, "2026-11-15extra", 10, "Valid reason"
        )
    );
}

// 22. Reject an extension when the extensions table is unavailable.
TEST_F(ExtensionRepositoryTest, RejectWhenExtensionsTableIsMissing) {
    database->execute("DROP TABLE extensions;");

    EXPECT_FALSE(
        repository->grantIndividualExtension(
            500, 20, "2026-11-15", 10, "Valid reason"
        )
    );
}
