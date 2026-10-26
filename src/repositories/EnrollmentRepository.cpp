
#include "repositories/EnrollmentRepository.h"

EnrollmentRepository::EnrollmentRepository(sqlite3* database)
    : db(database) {
}

bool EnrollmentRepository::isStudent(int studentId) const {
    if (db == nullptr || studentId <= 0) {
        return false;
    }

    const char* sql =
        "SELECT 1 "
        "FROM users "
        "WHERE user_id = ? AND role = 'Student' "
        "LIMIT 1;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr)
        != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(statement, 1, studentId);

    bool found = (sqlite3_step(statement) == SQLITE_ROW);

    sqlite3_finalize(statement);

    return found;
}

bool EnrollmentRepository::courseExists(int courseId) const {
    if (db == nullptr || courseId <= 0) {
        return false;
    }

    const char* sql =
        "SELECT 1 "
        "FROM courses "
        "WHERE course_id = ? "
        "LIMIT 1;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr)
        != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(statement, 1, courseId);

    bool found = (sqlite3_step(statement) == SQLITE_ROW);

    sqlite3_finalize(statement);

    return found;
}

bool EnrollmentRepository::isStudentEnrolled(
    int studentId,
    int courseId
) const {
    if (db == nullptr || studentId <= 0 || courseId <= 0) {
        return false;
    }

    const char* sql =
        "SELECT 1 "
        "FROM enrollments "
        "WHERE student_id = ? AND course_id = ? "
        "LIMIT 1;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr)
        != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(statement, 1, studentId);
    sqlite3_bind_int(statement, 2, courseId);

    bool enrolled = (sqlite3_step(statement) == SQLITE_ROW);

    sqlite3_finalize(statement);

    return enrolled;
}

bool EnrollmentRepository::enrollStudent(
    int studentId,
    int courseId
) {
    if (db == nullptr) {
        return false;
    }

    if (!isStudent(studentId) || !courseExists(courseId)) {
        return false;
    }

    if (isStudentEnrolled(studentId, courseId)) {
        return false;
    }

    const char* sql =
        "INSERT INTO enrollments (student_id, course_id) "
        "VALUES (?, ?);";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr)
        != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(statement, 1, studentId);
    sqlite3_bind_int(statement, 2, courseId);

    bool success = (sqlite3_step(statement) == SQLITE_DONE);

    sqlite3_finalize(statement);

    return success;
}

std::vector<int> EnrollmentRepository::getEnrolledCourses(
    int studentId
) const {
    std::vector<int> courseIds;

    if (!isStudent(studentId)) {
        return courseIds;
    }

    const char* sql =
        "SELECT course_id "
        "FROM enrollments "
        "WHERE student_id = ? "
        "ORDER BY course_id;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr)
        != SQLITE_OK) {
        return courseIds;
    }

    sqlite3_bind_int(statement, 1, studentId);

    while (sqlite3_step(statement) == SQLITE_ROW) {
        courseIds.push_back(sqlite3_column_int(statement, 0));
    }

    sqlite3_finalize(statement);

    return courseIds;
}

std::vector<int> EnrollmentRepository::getAvailableCourses(
    int studentId
) const {
    std::vector<int> courseIds;

    if (!isStudent(studentId)) {
        return courseIds;
    }

    const char* sql =
        "SELECT c.course_id "
        "FROM courses c "
        "WHERE NOT EXISTS ("
        "    SELECT 1 "
        "    FROM enrollments e "
        "    WHERE e.student_id = ? "
        "      AND e.course_id = c.course_id"
        ") "
        "ORDER BY c.course_id;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr)
        != SQLITE_OK) {
        return courseIds;
    }

    sqlite3_bind_int(statement, 1, studentId);

    while (sqlite3_step(statement) == SQLITE_ROW) {
        courseIds.push_back(sqlite3_column_int(statement, 0));
    }

    sqlite3_finalize(statement);

    return courseIds;
}
