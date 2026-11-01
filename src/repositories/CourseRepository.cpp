
#include "repositories/CourseRepository.h"

#include <sqlite3.h>

#include <stdexcept>
#include <string>
#include <vector>

CourseRepository::CourseRepository(Database& database)
    : database(database) {
}

bool CourseRepository::addCourse(const Course& course) {
    const char* sql =
        "INSERT INTO courses "
        "(course_code, course_name, department_id) "
        "VALUES (?, ?, ?);";

    sqlite3* connection = database.getConnection();
    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        connection,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection));
    }

    result = sqlite3_bind_text(
        statement,
        1,
        course.getCourseCode().c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    if (result == SQLITE_OK) {
        result = sqlite3_bind_text(
            statement,
            2,
            course.getCourseName().c_str(),
            -1,
            SQLITE_TRANSIENT
        );
    }

    if (result == SQLITE_OK) {
        result = sqlite3_bind_int(
            statement,
            3,
            course.getDepartmentId()
        );
    }

    if (result != SQLITE_OK) {
        std::string error = sqlite3_errmsg(connection);
        sqlite3_finalize(statement);
        throw std::runtime_error(error);
    }

    result = sqlite3_step(statement);

    std::string error;

    if (result != SQLITE_DONE &&
        (result & 0xFF) != SQLITE_CONSTRAINT) {
        error = sqlite3_errmsg(connection);
    }

    sqlite3_finalize(statement);

    if (result == SQLITE_DONE) {
        return true;
    }

    if ((result & 0xFF) == SQLITE_CONSTRAINT) {
        return false;
    }

    throw std::runtime_error(error);
}

std::vector<Course> CourseRepository::getAllCourses() {
    const char* sql =
        "SELECT course_id, course_code, course_name, department_id "
        "FROM courses ORDER BY course_code;";

    sqlite3* connection = database.getConnection();
    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            connection, sql, -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection));
    }

    std::vector<Course> courses;

    while (true) {
        int result = sqlite3_step(statement);

        if (result == SQLITE_DONE) {
            break;
        }

        if (result != SQLITE_ROW) {
            std::string error = sqlite3_errmsg(connection);
            sqlite3_finalize(statement);
            throw std::runtime_error(error);
        }

        const auto* code = sqlite3_column_text(statement, 1);
        const auto* name = sqlite3_column_text(statement, 2);

        courses.emplace_back(
            sqlite3_column_int(statement, 0),
            code ? reinterpret_cast<const char*>(code) : "",
            name ? reinterpret_cast<const char*>(name) : "",
            sqlite3_column_int(statement, 3)
        );
    }

    sqlite3_finalize(statement);
    return courses;
}

/*
 * Assigns a lecturer to a course.
 */
bool CourseRepository::assignLecturerToCourse(
    int lecturerId,
    int courseId) {

    const char* sql =
        "INSERT INTO course_lecturers (lecturer_id, course_id) "
        "SELECT u.user_id, c.course_id "
        "FROM users u "
        "JOIN courses c ON c.course_id = ? "
        "WHERE u.user_id = ? "
        "AND u.role = 'Lecturer' "
        "AND u.department_id = c.department_id;";

    sqlite3* connection = database.getConnection();
    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            connection, sql, -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection));
    }

    sqlite3_bind_int(statement, 1, courseId);
    sqlite3_bind_int(statement, 2, lecturerId);

    int result = sqlite3_step(statement);
    int rowsInserted = sqlite3_changes(connection);

    std::string error;

    if (result != SQLITE_DONE) {
        error = sqlite3_errmsg(connection);
    }

    sqlite3_finalize(statement);

    if (result != SQLITE_DONE) {
        if (error.find("UNIQUE constraint failed") != std::string::npos ||
            error.find("PRIMARY KEY constraint failed") != std::string::npos) {
            return false;
        }

        throw std::runtime_error(error);
    }

    // INSERT ... SELECT may complete without inserting any rows
    // if the lecturer, course, or department validation fails.
    return rowsInserted == 1;
}

/* Returns the IDs of lecturers assigned to the specified course. */
std::vector<int> CourseRepository::getLecturersForCourse(int courseId) {
    const char* sql =
        "SELECT lecturer_id "
        "FROM course_lecturers "
        "WHERE course_id = ? "
        "ORDER BY lecturer_id;";

    sqlite3* connection = database.getConnection();
    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            connection, sql, -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection));
    }

    sqlite3_bind_int(statement, 1, courseId);

    std::vector<int> lecturerIds;

    while (true) {
        int result = sqlite3_step(statement);

        if (result == SQLITE_DONE) {
            break;
        }

        if (result != SQLITE_ROW) {
            std::string error = sqlite3_errmsg(connection);
            sqlite3_finalize(statement);
            throw std::runtime_error(error);
        }

        lecturerIds.push_back(sqlite3_column_int(statement, 0));
    }

    sqlite3_finalize(statement);
    return lecturerIds;
}

