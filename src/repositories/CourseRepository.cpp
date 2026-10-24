#include "repositories/CourseRepository.h"

#include <sqlite3.h>
#include <stdexcept>

CourseRepository::CourseRepository(Database& database)
    : database(database) {}

bool CourseRepository::addCourse(const Course& course) {
    const char* sql =
        "INSERT INTO courses "
        "(course_code, course_name, department_id) "
        "VALUES (?, ?, ?);";

    sqlite3_stmt* statement = nullptr;
    sqlite3* connection = database.getConnection();

    if (sqlite3_prepare_v2(
            connection, sql, -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection));
    }

    sqlite3_bind_text(
        statement, 1, course.getCourseCode().c_str(),
        -1, SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement, 2, course.getCourseName().c_str(),
        -1, SQLITE_TRANSIENT
    );

    sqlite3_bind_int(
        statement, 3, course.getDepartmentId()
    );

    int result = sqlite3_step(statement);
    sqlite3_finalize(statement);

    if (result == SQLITE_DONE) {
        return true;
    }

    if (result == SQLITE_CONSTRAINT) {
        return false;
    }

    throw std::runtime_error(sqlite3_errmsg(connection));
}

std::vector<Course> CourseRepository::getAllCourses() {
    const char* sql =
        "SELECT course_id, course_code, course_name, department_id "
        "FROM courses ORDER BY course_code;";

    sqlite3_stmt* statement = nullptr;
    sqlite3* connection = database.getConnection();

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

        courses.emplace_back(
            sqlite3_column_int(statement, 0),
            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 1)),
            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 2)),
            sqlite3_column_int(statement, 3)
        );
    }

    sqlite3_finalize(statement);
    return courses;
}