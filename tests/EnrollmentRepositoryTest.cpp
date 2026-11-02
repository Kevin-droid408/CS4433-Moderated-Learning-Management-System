#include <gtest/gtest.h>
#include <sqlite3.h>

#include <vector>

#include "repositories/EnrollmentRepository.h"

class EnrollmentRepositoryTest : public ::testing::Test {
protected:
    sqlite3* db = nullptr;
    EnrollmentRepository* repository = nullptr;

    void SetUp() override {
        ASSERT_EQ(sqlite3_open(":memory:", &db), SQLITE_OK);

        createTables();
        insertTestData();

        repository = new EnrollmentRepository(db);
    }

    void TearDown() override {
        delete repository;
        repository = nullptr;

        if (db != nullptr) {
            sqlite3_close(db);
            db = nullptr;
        }
    }

    void executeSQL(const char* sql) {
        char* errorMessage = nullptr;

        int result = sqlite3_exec(
            db,
            sql,
            nullptr,
            nullptr,
            &errorMessage
        );

        if (errorMessage != nullptr) {
            sqlite3_free(errorMessage);
        }

        ASSERT_EQ(result, SQLITE_OK);
    }

    void createTables() {
        executeSQL(
            "CREATE TABLE users ("
            "user_id INTEGER PRIMARY KEY,"
            "role TEXT NOT NULL"
            ");"
        );

        executeSQL(
            "CREATE TABLE courses ("
            "course_id INTEGER PRIMARY KEY"
            ");"
        );

        executeSQL(
            "CREATE TABLE enrollments ("
            "student_id INTEGER NOT NULL,"
            "course_id INTEGER NOT NULL,"
            "PRIMARY KEY (student_id, course_id)"
            ");"
        );
    }

    void insertTestData() {
        executeSQL(
            "INSERT INTO users (user_id, role) VALUES "
            "(1, 'Student'),"
            "(2, 'Student'),"
            "(3, 'Lecturer');"
        );

        executeSQL(
            "INSERT INTO courses (course_id) VALUES "
            "(101),"
            "(102),"
            "(103),"
            "(104);"
        );
    }
};

// 1. Successful enrollment
TEST_F(EnrollmentRepositoryTest, EnrollStudentSuccessfully) {
    EXPECT_TRUE(repository->enrollStudent(1, 101));

    EXPECT_TRUE(repository->isStudentEnrolled(1, 101));
}

// 2. Reject invalid student ID
TEST_F(EnrollmentRepositoryTest, RejectInvalidStudentId) {
    EXPECT_FALSE(repository->enrollStudent(0, 101));
    EXPECT_FALSE(repository->enrollStudent(-1, 101));
}

// 3. Reject non-existent student
TEST_F(EnrollmentRepositoryTest, RejectNonExistentStudent) {
    EXPECT_FALSE(repository->enrollStudent(999, 101));
}

// 4. Reject users who are not students
TEST_F(EnrollmentRepositoryTest, RejectNonStudentUser) {
    EXPECT_FALSE(repository->enrollStudent(3, 101));
}

// 5. Reject invalid course ID
TEST_F(EnrollmentRepositoryTest, RejectInvalidCourseId) {
    EXPECT_FALSE(repository->enrollStudent(1, 0));
    EXPECT_FALSE(repository->enrollStudent(1, -1));
}

// 6. Reject non-existent course
TEST_F(EnrollmentRepositoryTest, RejectNonExistentCourse) {
    EXPECT_FALSE(repository->enrollStudent(1, 999));
}

// 7. Prevent duplicate enrollment
TEST_F(EnrollmentRepositoryTest, RejectDuplicateEnrollment) {
    EXPECT_TRUE(repository->enrollStudent(1, 101));

    EXPECT_FALSE(repository->enrollStudent(1, 101));
}

// 8. Check enrollment status
TEST_F(EnrollmentRepositoryTest, CheckEnrollmentStatus) {
    EXPECT_FALSE(repository->isStudentEnrolled(1, 101));

    EXPECT_TRUE(repository->enrollStudent(1, 101));

    EXPECT_TRUE(repository->isStudentEnrolled(1, 101));
    EXPECT_FALSE(repository->isStudentEnrolled(1, 102));
}

// 9. Reject invalid IDs when checking enrollment
TEST_F(EnrollmentRepositoryTest, RejectInvalidEnrollmentCheck) {
    EXPECT_FALSE(repository->isStudentEnrolled(0, 101));
    EXPECT_FALSE(repository->isStudentEnrolled(1, 0));
    EXPECT_FALSE(repository->isStudentEnrolled(-1, -1));
}

// 10. Retrieve enrolled courses
TEST_F(EnrollmentRepositoryTest, GetEnrolledCourses) {
    EXPECT_TRUE(repository->enrollStudent(1, 103));
    EXPECT_TRUE(repository->enrollStudent(1, 101));
    EXPECT_TRUE(repository->enrollStudent(1, 102));

    std::vector<int> courses =
        repository->getEnrolledCourses(1);

    ASSERT_EQ(courses.size(), 3);

    EXPECT_EQ(courses[0], 101);
    EXPECT_EQ(courses[1], 102);
    EXPECT_EQ(courses[2], 103);
}

// 11. Return empty list when student has no enrollments
TEST_F(EnrollmentRepositoryTest, GetEnrolledCoursesWhenEmpty) {
    std::vector<int> courses =
        repository->getEnrolledCourses(1);

    EXPECT_TRUE(courses.empty());
}

// 12. Return empty list for invalid student
TEST_F(EnrollmentRepositoryTest, GetEnrolledCoursesForInvalidStudent) {
    EXPECT_TRUE(
        repository->getEnrolledCourses(999).empty()
    );

    EXPECT_TRUE(
        repository->getEnrolledCourses(3).empty()
    );
}

// 13. Retrieve available courses
TEST_F(EnrollmentRepositoryTest, GetAvailableCourses) {
    EXPECT_TRUE(repository->enrollStudent(1, 101));
    EXPECT_TRUE(repository->enrollStudent(1, 103));

    std::vector<int> courses =
        repository->getAvailableCourses(1);

    ASSERT_EQ(courses.size(), 2);

    EXPECT_EQ(courses[0], 102);
    EXPECT_EQ(courses[1], 104);
}

// 14. Return all courses when student has no enrollments
TEST_F(EnrollmentRepositoryTest, GetAvailableCoursesWhenNotEnrolled) {
    std::vector<int> courses =
        repository->getAvailableCourses(1);

    ASSERT_EQ(courses.size(), 4);

    EXPECT_EQ(courses[0], 101);
    EXPECT_EQ(courses[1], 102);
    EXPECT_EQ(courses[2], 103);
    EXPECT_EQ(courses[3], 104);
}

// 15. Return empty available courses for invalid student
TEST_F(EnrollmentRepositoryTest, GetAvailableCoursesForInvalidStudent) {
    EXPECT_TRUE(
        repository->getAvailableCourses(999).empty()
    );

    EXPECT_TRUE(
        repository->getAvailableCourses(3).empty()
    );
}

// 16. Handle null database connection
TEST(EnrollmentRepositoryNullDatabaseTest, RejectOperations) {
    EnrollmentRepository repository(nullptr);

    EXPECT_FALSE(repository.enrollStudent(1, 101));

    EXPECT_FALSE(
        repository.isStudentEnrolled(1, 101)
    );

    EXPECT_TRUE(
        repository.getEnrolledCourses(1).empty()
    );

    EXPECT_TRUE(
        repository.getAvailableCourses(1).empty()
    );
}