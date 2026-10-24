
#include "repositories/AssignmentRepository.h"

#include <sqlite3.h>
#include <stdexcept>
#include <string>

AssignmentRepository::AssignmentRepository(Database& database)
    : database(database) {}

std::vector<Assignment> AssignmentRepository::getAllAssignments() {
    const char* sql =
        "SELECT assignment_id, course_id, title, description, "
        "due_date, max_mark "
        "FROM assignments "
        "ORDER BY due_date, title;";

    sqlite3_stmt* statement = nullptr;
    sqlite3* connection = database.getConnection();

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

        assignments.emplace_back(
            sqlite3_column_int(statement, 0),
            sqlite3_column_int(statement, 1),
            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 2)),
            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 3)),
            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 4)),
            sqlite3_column_double(statement, 5)
        );
    }

    sqlite3_finalize(statement);
    return assignments;
}

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

        assignments.emplace_back(
            sqlite3_column_int(statement, 0),
            sqlite3_column_int(statement, 1),
            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 2)),
            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 3)),
            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 4)),
            sqlite3_column_double(statement, 5)
        );
    }

    sqlite3_finalize(statement);
    return assignments;
}
