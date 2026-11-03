#include "repositories/AssignmentRepository.h"

#include <sqlite3.h>
#include <stdexcept>
#include <string>

AssignmentRepository::AssignmentRepository(Database& database)
    : database(database) {}

/* Adds a new assignment to the database. */
bool AssignmentRepository::addAssignment(
    const Assignment& assignment) {

    const char* sql =
        "INSERT INTO assignments "
        "(course_id, title, description, due_date, max_mark) "
        "VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt* statement = nullptr;
    sqlite3* connection = database.getConnection();

    if (connection == nullptr) {
        throw std::runtime_error("Database connection is unavailable.");
    }

    if (sqlite3_prepare_v2(
            connection, sql, -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection));
    }

    int result = sqlite3_bind_int(
        statement, 1, assignment.getCourseId());

    if (result == SQLITE_OK) {
        result = sqlite3_bind_text(
            statement, 2, assignment.getTitle().c_str(),
            -1, SQLITE_TRANSIENT);
    }

    if (result == SQLITE_OK) {
        result = sqlite3_bind_text(
            statement, 3, assignment.getDescription().c_str(),
            -1, SQLITE_TRANSIENT);
    }

    if (result == SQLITE_OK) {
        result = sqlite3_bind_text(
            statement, 4, assignment.getDueDate().c_str(),
            -1, SQLITE_TRANSIENT);
    }

    if (result == SQLITE_OK) {
        result = sqlite3_bind_double(
            statement, 5, assignment.getMaxMark());
    }

    if (result != SQLITE_OK) {
        std::string error = sqlite3_errmsg(connection);
        sqlite3_finalize(statement);
        throw std::runtime_error(error);
    }

    result = sqlite3_step(statement);

    if (result == SQLITE_DONE) {
        sqlite3_finalize(statement);
        return true;
    }

    std::string error = sqlite3_errmsg(connection);
    int errorCode = result;

    sqlite3_finalize(statement);

    if (errorCode == SQLITE_CONSTRAINT ||
        errorCode == SQLITE_CONSTRAINT_FOREIGNKEY ||
        errorCode == SQLITE_CONSTRAINT_CHECK) {
        return false;
    }

    throw std::runtime_error(error);
}

/* Retrieves every assignment. */
std::vector<Assignment>
AssignmentRepository::getAllAssignments() {

    const char* sql =
        "SELECT assignment_id, course_id, title, description, "
        "due_date, max_mark "
        "FROM assignments "
        "ORDER BY due_date, title;";

    sqlite3_stmt* statement = nullptr;
    sqlite3* connection = database.getConnection();

    if (connection == nullptr) {
        throw std::runtime_error("Database connection is unavailable.");
    }

    if (sqlite3_prepare_v2(
            connection, sql, -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection));
    }

    std::vector<Assignment> assignments;

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

        const auto* title = sqlite3_column_text(statement, 2);
        const auto* description = sqlite3_column_text(statement, 3);
        const auto* dueDate = sqlite3_column_text(statement, 4);

        assignments.emplace_back(
            sqlite3_column_int(statement, 0),
            sqlite3_column_int(statement, 1),
            title ? reinterpret_cast<const char*>(title) : "",
            description ? reinterpret_cast<const char*>(description) : "",
            dueDate ? reinterpret_cast<const char*>(dueDate) : "",
            sqlite3_column_double(statement, 5)
        );
    }

    sqlite3_finalize(statement);
    return assignments;
}

/* Retrieves assignments belonging to a particular course. */
std::vector<Assignment>
AssignmentRepository::getAssignmentsByCourse(int courseId) {

    const char* sql =
        "SELECT assignment_id, course_id, title, description, "
        "due_date, max_mark "
        "FROM assignments "
        "WHERE course_id = ? "
        "ORDER BY due_date, title;";

    sqlite3_stmt* statement = nullptr;
    sqlite3* connection = database.getConnection();

    if (connection == nullptr || courseId <= 0) {
        return {};
    }

    if (sqlite3_prepare_v2(
            connection, sql, -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection));
    }

    if (sqlite3_bind_int(statement, 1, courseId) != SQLITE_OK) {
        std::string error = sqlite3_errmsg(connection);
        sqlite3_finalize(statement);
        throw std::runtime_error(error);
    }

    std::vector<Assignment> assignments;

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

        const auto* title = sqlite3_column_text(statement, 2);
        const auto* description = sqlite3_column_text(statement, 3);
        const auto* dueDate = sqlite3_column_text(statement, 4);

        assignments.emplace_back(
            sqlite3_column_int(statement, 0),
            sqlite3_column_int(statement, 1),
            title ? reinterpret_cast<const char*>(title) : "",
            description ? reinterpret_cast<const char*>(description) : "",
            dueDate ? reinterpret_cast<const char*>(dueDate) : "",
            sqlite3_column_double(statement, 5)
        );
    }

    sqlite3_finalize(statement);
    return assignments;
}

/*
 * Retrieves assignments for a particular student.
 * If an individual extension exists, its revised deadline is returned.
 * Otherwise, the original assignment deadline is returned.
 */
std::vector<Assignment>
AssignmentRepository::getAssignmentsForStudent(
    int courseId,
    int studentId) {

    std::vector<Assignment> assignments;

    sqlite3* connection = database.getConnection();

    if (connection == nullptr || courseId <= 0 || studentId <= 0) {
        return assignments;
    }

    const char* sql =
        "SELECT "
        "a.assignment_id, "
        "a.course_id, "
        "a.title, "
        "a.description, "
        "COALESCE(e.new_due_date, a.due_date) AS due_date, "
        "a.max_mark "
        "FROM assignments a "
        "LEFT JOIN extensions e "
        "ON e.assignment_id = a.assignment_id "
        "AND e.student_id = ? "
        "WHERE a.course_id = ? "
        "ORDER BY due_date, a.title;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            connection, sql, -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error(
            "Failed to retrieve assignments for student: " +
            std::string(sqlite3_errmsg(connection))
        );
    }

    int result = sqlite3_bind_int(statement, 1, studentId);

    if (result == SQLITE_OK) {
        result = sqlite3_bind_int(statement, 2, courseId);
    }

    if (result != SQLITE_OK) {
        std::string error = sqlite3_errmsg(connection);
        sqlite3_finalize(statement);
        throw std::runtime_error(error);
    }

    while (true) {
        result = sqlite3_step(statement);

        if (result == SQLITE_DONE) {
            break;
        }

        if (result != SQLITE_ROW) {
            std::string error = sqlite3_errmsg(connection);
            sqlite3_finalize(statement);
            throw std::runtime_error(
                "Error while reading student assignments: " + error
            );
        }

        const auto* title = sqlite3_column_text(statement, 2);
        const auto* description = sqlite3_column_text(statement, 3);
        const auto* dueDate = sqlite3_column_text(statement, 4);

        assignments.emplace_back(
            sqlite3_column_int(statement, 0),
            sqlite3_column_int(statement, 1),
            title ? reinterpret_cast<const char*>(title) : "",
            description ? reinterpret_cast<const char*>(description) : "",
            dueDate ? reinterpret_cast<const char*>(dueDate) : "",
            sqlite3_column_double(statement, 5)
        );
    }

    sqlite3_finalize(statement);

    return assignments;
}