
#include "repositories/CourseRepository.h"

#include <sqlite3.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

    // Automatically finalizes SQLite statements when they leave scope.
    using Statement = std::unique_ptr<
        sqlite3_stmt,
        decltype(&sqlite3_finalize)
    >;

    // Prepares an SQL statement and checks for errors.
    Statement prepareStatement(
        sqlite3* connection,
        const char* sql
    ) {
        if (connection == nullptr) {
            throw std::runtime_error("Database connection is invalid.");
        }

        sqlite3_stmt* rawStatement = nullptr;

        int result = sqlite3_prepare_v2(
            connection,
            sql,
            -1,
            &rawStatement,
            nullptr
        );

        if (result != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(connection));
        }

        return Statement(rawStatement, sqlite3_finalize);
    }

    // Checks whether an SQLite parameter binding succeeded.
    void checkBindResult(
        sqlite3* connection,
        int result
    ) {
        if (result != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(connection));
        }
    }

    // Checks whether an SQL operation completed successfully.
    void checkStepResult(
        sqlite3* connection,
        int result
    ) {
        if (result != SQLITE_DONE) {
            throw std::runtime_error(sqlite3_errmsg(connection));
        }
    }

}

// Constructor
CourseRepository::CourseRepository(Database& database)
    : database(database) {
}

// Adds a new course.
bool CourseRepository::addCourse(const Course& course) {

    const char* sql =
        "INSERT INTO courses "
        "(course_code, course_name, department_id) "
        "VALUES (?, ?, ?);";

    sqlite3* connection = database.getConnection();

    Statement statement = prepareStatement(
        connection,
        sql
    );

    // Bind course code.
    checkBindResult(
        connection,
        sqlite3_bind_text(
            statement.get(),
            1,
            course.getCourseCode().c_str(),
            -1,
            SQLITE_TRANSIENT
        )
    );

    // Bind course name.
    checkBindResult(
        connection,
        sqlite3_bind_text(
            statement.get(),
            2,
            course.getCourseName().c_str(),
            -1,
            SQLITE_TRANSIENT
        )
    );

    // Bind department ID.
    checkBindResult(
        connection,
        sqlite3_bind_int(
            statement.get(),
            3,
            course.getDepartmentId()
        )
    );

    int result = sqlite3_step(statement.get());

    if (result == SQLITE_DONE) {
        return true;
    }

    // Return false for database constraint violations,
    // such as duplicate course codes or invalid department IDs.
    if ((result & 0xFF) == SQLITE_CONSTRAINT) {
        return false;
    }

    throw std::runtime_error(sqlite3_errmsg(connection));
}

// Retrieves all courses ordered by course code.
std::vector<Course> CourseRepository::getAllCourses() {

    const char* sql =
        "SELECT course_id, course_code, course_name, department_id "
        "FROM courses "
        "ORDER BY course_code;";

    sqlite3* connection = database.getConnection();

    Statement statement = prepareStatement(
        connection,
        sql
    );

    std::vector<Course> courses;

    while (true) {

        int result = sqlite3_step(statement.get());

        if (result == SQLITE_DONE) {
            break;
        }

        if (result != SQLITE_ROW) {
            throw std::runtime_error(sqlite3_errmsg(connection));
        }

        const auto* code =
            sqlite3_column_text(statement.get(), 1);

        const auto* name =
            sqlite3_column_text(statement.get(), 2);

        courses.emplace_back(
            sqlite3_column_int(statement.get(), 0),

            code
                ? reinterpret_cast<const char*>(code)
                : "",

            name
                ? reinterpret_cast<const char*>(name)
                : "",

            sqlite3_column_int(statement.get(), 3)
        );
    }

    return courses;
}

// Assigns a lecturer to a course.
// The lecturer and course must belong to the same department.
bool CourseRepository::assignLecturerToCourse(
    int lecturerId,
    int courseId
) {

    const char* sql =
        "INSERT INTO course_lecturers (lecturer_id, course_id) "
        "SELECT u.user_id, c.course_id "
        "FROM users u "
        "JOIN courses c ON c.course_id = ? "
        "WHERE u.user_id = ? "
        "AND u.role = 'Lecturer' "
        "AND u.department_id = c.department_id;";

    sqlite3* connection = database.getConnection();

    Statement statement = prepareStatement(
        connection,
        sql
    );

    // Bind course ID.
    checkBindResult(
        connection,
        sqlite3_bind_int(
            statement.get(),
            1,
            courseId
        )
    );

    // Bind lecturer ID.
    checkBindResult(
        connection,
        sqlite3_bind_int(
            statement.get(),
            2,
            lecturerId
        )
    );

    int result = sqlite3_step(statement.get());

    // Return false for constraint violations, including
    // duplicate assignments and foreign-key violations.
    if ((result & 0xFF) == SQLITE_CONSTRAINT) {
        return false;
    }

    if (result != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(connection));
    }

    // INSERT ... SELECT can finish successfully without inserting
    // a row if the lecturer or course does not satisfy the conditions.
    int rowsInserted = sqlite3_changes(connection);

    return rowsInserted == 1;
}

// Retrieves lecturer IDs assigned to a particular course.
std::vector<int> CourseRepository::getLecturersForCourse(
    int courseId
) {

    const char* sql =
        "SELECT lecturer_id "
        "FROM course_lecturers "
        "WHERE course_id = ? "
        "ORDER BY lecturer_id;";

    sqlite3* connection = database.getConnection();

    Statement statement = prepareStatement(
        connection,
        sql
    );

    // Bind course ID.
    checkBindResult(
        connection,
        sqlite3_bind_int(
            statement.get(),
            1,
            courseId
        )
    );

    std::vector<int> lecturerIds;

    while (true) {

        int result = sqlite3_step(statement.get());

        if (result == SQLITE_DONE) {
            break;
        }

        if (result != SQLITE_ROW) {
            throw std::runtime_error(sqlite3_errmsg(connection));
        }

        lecturerIds.push_back(
            sqlite3_column_int(statement.get(), 0)
        );
    }

    return lecturerIds;
}
