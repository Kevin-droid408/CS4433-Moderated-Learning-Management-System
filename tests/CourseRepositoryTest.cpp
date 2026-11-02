
#include <gtest/gtest.h>

#include "repositories/Database.h"
#include "repositories/CourseRepository.h"
#include "models/Course.h"

#include <sqlite3.h>

#include <vector>

class CourseRepositoryTest : public ::testing::Test {
protected:

    // Each test receives its own in-memory database.
    Database database{":memory:"};

    // Repository uses the database connection.
    CourseRepository courseRepository{database};

    void SetUp() override {

        // Enable foreign-key enforcement directly through SQLite.
        int result = sqlite3_exec(
            database.getConnection(),
            "PRAGMA foreign_keys = ON;",
            nullptr,
            nullptr,
            nullptr
        );

        ASSERT_EQ(result, SQLITE_OK);

        // Verify that foreign-key enforcement is enabled.
        sqlite3_stmt* statement = nullptr;

        result = sqlite3_prepare_v2(
            database.getConnection(),
            "PRAGMA foreign_keys;",
            -1,
            &statement,
            nullptr
        );

        ASSERT_EQ(result, SQLITE_OK);

        result = sqlite3_step(statement);

        ASSERT_EQ(result, SQLITE_ROW);
        EXPECT_EQ(sqlite3_column_int(statement, 0), 1);

        sqlite3_finalize(statement);

        // Create departments table.
        database.execute(
            "CREATE TABLE departments ("
            "department_id INTEGER PRIMARY KEY, "
            "department_name TEXT NOT NULL"
            ");"
        );

        // Create users table.
        database.execute(
            "CREATE TABLE users ("
            "user_id INTEGER PRIMARY KEY, "
            "username TEXT NOT NULL, "
            "role TEXT NOT NULL, "
            "department_id INTEGER NOT NULL, "
            "FOREIGN KEY (department_id) "
            "REFERENCES departments(department_id)"
            ");"
        );

        // Create courses table.
        database.execute(
            "CREATE TABLE courses ("
            "course_id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "course_code TEXT NOT NULL UNIQUE, "
            "course_name TEXT NOT NULL, "
            "department_id INTEGER NOT NULL, "
            "FOREIGN KEY (department_id) "
            "REFERENCES departments(department_id)"
            ");"
        );

        // Create course_lecturers table.
        database.execute(
            "CREATE TABLE course_lecturers ("
            "lecturer_id INTEGER NOT NULL, "
            "course_id INTEGER NOT NULL, "
            "PRIMARY KEY (lecturer_id, course_id), "
            "FOREIGN KEY (lecturer_id) "
            "REFERENCES users(user_id), "
            "FOREIGN KEY (course_id) "
            "REFERENCES courses(course_id)"
            ");"
        );

        // Insert test departments.
        database.execute(
            "INSERT INTO departments VALUES "
            "(1, 'Computer Science'), "
            "(2, 'Information Technology');"
        );

        // Insert test users.
        database.execute(
            "INSERT INTO users VALUES "
            "(1, 'lecturer1', 'Lecturer', 1), "
            "(2, 'lecturer2', 'Lecturer', 2), "
            "(3, 'student1', 'Student', 1);"
        );
    }

    // Helper function to add a course and return its database ID.
    int addTestCourse(
        const std::string& code = "CS101",
        const std::string& name = "Programming",
        int departmentId = 1
    ) {

        Course course(
            0,
            code,
            name,
            departmentId
        );

        if (!courseRepository.addCourse(course)) {
            return -1;
        }

        std::vector<Course> courses =
            courseRepository.getAllCourses();

        for (const Course& existingCourse : courses) {

            if (existingCourse.getCourseCode() == code) {
                return existingCourse.getCourseId();
            }
        }

        return -1;
    }
};

// --------------------------------------------------
// COURSE INSERTION TESTS
// --------------------------------------------------

// Test 1: Successfully add a course.
TEST_F(CourseRepositoryTest, AddCourseSuccessfully) {

    Course course(
        0,
        "CS101",
        "Introduction to Computer Science",
        1
    );

    EXPECT_TRUE(courseRepository.addCourse(course));

    std::vector<Course> courses =
        courseRepository.getAllCourses();

    ASSERT_EQ(courses.size(), 1);

    EXPECT_EQ(courses[0].getCourseCode(), "CS101");

    EXPECT_EQ(
        courses[0].getCourseName(),
        "Introduction to Computer Science"
    );

    EXPECT_EQ(courses[0].getDepartmentId(), 1);
}

// Test 2: Reject duplicate course codes.
TEST_F(CourseRepositoryTest, RejectDuplicateCourseCode) {

    Course firstCourse(
        0, "CS101", "Computer Science", 1
    );

    Course secondCourse(
        0, "CS101", "Different Course", 1
    );

    EXPECT_TRUE(courseRepository.addCourse(firstCourse));

    EXPECT_FALSE(courseRepository.addCourse(secondCourse));

    EXPECT_EQ(courseRepository.getAllCourses().size(), 1);
}

// Test 3: Reject a course with a non-existent department.
TEST_F(CourseRepositoryTest, RejectInvalidDepartment) {

    Course course(
        0,
        "CS201",
        "Data Structures",
        99
    );

    EXPECT_FALSE(courseRepository.addCourse(course));

    EXPECT_TRUE(courseRepository.getAllCourses().empty());
}

// --------------------------------------------------
// COURSE RETRIEVAL TESTS
// --------------------------------------------------

// Test 4: Retrieve courses in course-code order.
TEST_F(CourseRepositoryTest, RetrieveCoursesInOrder) {

    ASSERT_NE(addTestCourse("CS301", "Algorithms"), -1);
    ASSERT_NE(addTestCourse("CS101", "Programming"), -1);
    ASSERT_NE(addTestCourse("CS201", "Databases"), -1);

    std::vector<Course> courses =
        courseRepository.getAllCourses();

    ASSERT_EQ(courses.size(), 3);

    EXPECT_EQ(courses[0].getCourseCode(), "CS101");
    EXPECT_EQ(courses[1].getCourseCode(), "CS201");
    EXPECT_EQ(courses[2].getCourseCode(), "CS301");
}

// Test 5: Return an empty list when no courses exist.
TEST_F(CourseRepositoryTest, ReturnEmptyCourseList) {

    std::vector<Course> courses =
        courseRepository.getAllCourses();

    EXPECT_TRUE(courses.empty());
}

// --------------------------------------------------
// LECTURER ASSIGNMENT TESTS
// --------------------------------------------------

// Test 6: Successfully assign a lecturer to a course.
TEST_F(CourseRepositoryTest, AssignLecturerSuccessfully) {

    int courseId = addTestCourse();

    ASSERT_NE(courseId, -1);

    EXPECT_TRUE(
        courseRepository.assignLecturerToCourse(1, courseId)
    );

    std::vector<int> lecturers =
        courseRepository.getLecturersForCourse(courseId);

    ASSERT_EQ(lecturers.size(), 1);

    EXPECT_EQ(lecturers[0], 1);
}

// Test 7: Reject a lecturer from another department.
TEST_F(
    CourseRepositoryTest,
    RejectLecturerFromDifferentDepartment
) {

    int courseId = addTestCourse();

    ASSERT_NE(courseId, -1);

    EXPECT_FALSE(
        courseRepository.assignLecturerToCourse(2, courseId)
    );

    EXPECT_TRUE(
        courseRepository.getLecturersForCourse(courseId).empty()
    );
}

// Test 8: Reject a user who is not a lecturer.
TEST_F(CourseRepositoryTest, RejectStudentAsLecturer) {

    int courseId = addTestCourse();

    ASSERT_NE(courseId, -1);

    EXPECT_FALSE(
        courseRepository.assignLecturerToCourse(3, courseId)
    );
}

// Test 9: Reject a non-existent lecturer.
TEST_F(CourseRepositoryTest, RejectNonExistentLecturer) {

    int courseId = addTestCourse();

    ASSERT_NE(courseId, -1);

    EXPECT_FALSE(
        courseRepository.assignLecturerToCourse(99, courseId)
    );
}

// Test 10: Reject a duplicate lecturer assignment.
TEST_F(CourseRepositoryTest, RejectDuplicateLecturerAssignment) {

    int courseId = addTestCourse();

    ASSERT_NE(courseId, -1);

    ASSERT_TRUE(
        courseRepository.assignLecturerToCourse(1, courseId)
    );

    EXPECT_FALSE(
        courseRepository.assignLecturerToCourse(1, courseId)
    );

    EXPECT_EQ(
        courseRepository.getLecturersForCourse(courseId).size(),
        1
    );
}

// Test 11: Reject assignment to a non-existent course.
TEST_F(CourseRepositoryTest, RejectAssignmentToNonExistentCourse) {

    EXPECT_FALSE(
        courseRepository.assignLecturerToCourse(1, 999)
    );
}

// --------------------------------------------------
// LECTURER RETRIEVAL TESTS
// --------------------------------------------------

// Test 12: Return an empty list when a course has no lecturers.
TEST_F(CourseRepositoryTest, ReturnEmptyLecturerList) {

    int courseId = addTestCourse();

    ASSERT_NE(courseId, -1);

    std::vector<int> lecturers =
        courseRepository.getLecturersForCourse(courseId);

    EXPECT_TRUE(lecturers.empty());
}

// Test 13: Retrieve multiple lecturers assigned to a course.
TEST_F(CourseRepositoryTest, RetrieveMultipleLecturers) {

    int courseId = addTestCourse();

    ASSERT_NE(courseId, -1);

    // Add another lecturer to the same department.
    database.execute(
        "INSERT INTO users VALUES "
        "(4, 'lecturer3', 'Lecturer', 1);"
    );

    ASSERT_TRUE(
        courseRepository.assignLecturerToCourse(1, courseId)
    );

    ASSERT_TRUE(
        courseRepository.assignLecturerToCourse(4, courseId)
    );

    std::vector<int> lecturers =
        courseRepository.getLecturersForCourse(courseId);

    ASSERT_EQ(lecturers.size(), 2);

    // IDs should be returned in ascending order.
    EXPECT_EQ(lecturers[0], 1);
    EXPECT_EQ(lecturers[1], 4);
}
