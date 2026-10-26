
#include "repositories/ExtensionRepository.h"

#include <sqlite3.h>

#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace {

// Validates a real calendar date in YYYY-MM-DD format.
bool isValidDate(const std::string& value) {
    if (value.size() != 10 ||
        value[4] != '-' ||
        value[7] != '-') {
        return false;
    }

    std::istringstream input(value);
    std::tm date{};

    input >> std::get_time(&date, "%Y-%m-%d");

    if (input.fail()) {
        return false;
    }

    char extra;
    if (input >> extra) {
        return false;
    }

    int year = date.tm_year;
    int month = date.tm_mon;
    int day = date.tm_mday;

    date.tm_hour = 12;
    date.tm_isdst = -1;

    std::time_t timestamp = std::mktime(&date);

    if (timestamp == static_cast<std::time_t>(-1)) {
        return false;
    }

    std::tm normalized{};

#ifdef _WIN32
    if (localtime_s(&normalized, &timestamp) != 0) {
        return false;
    }
#else
    if (localtime_r(&timestamp, &normalized) == nullptr) {
        return false;
    }
#endif

    return normalized.tm_year == year &&
           normalized.tm_mon == month &&
           normalized.tm_mday == day;
}

// Checks that the reason is not empty or whitespace-only.
bool hasValidReason(const std::string& reason) {
    return reason.find_first_not_of(" \t\r\n") !=
           std::string::npos;
}

} // namespace


ExtensionRepository::ExtensionRepository(Database& database)
    : database(database) {
}


// Grants or updates an individual student's extension.
// The lecturer must belong to the assignment's department.
// The student must be enrolled in the assignment's course.
bool ExtensionRepository::grantIndividualExtension(
    int assignmentId,
    int studentId,
    const std::string& newDueDate,
    int lecturerId,
    const std::string& reason
) {
    sqlite3* db = database.getConnection();

    if (db == nullptr ||
        assignmentId <= 0 ||
        studentId <= 0 ||
        lecturerId <= 0 ||
        !isValidDate(newDueDate) ||
        !hasValidReason(reason)) {
        return false;
    }

    // Verify lecturer authorization, student enrollment,
    // and that the new deadline is later than the original.
    const char* authorizationSql =
        "SELECT 1 "
        "FROM assignments a "
        "JOIN courses c ON c.course_id = a.course_id "
        "JOIN users l ON l.department_id = c.department_id "
        "JOIN users s ON s.user_id = ? "
        "JOIN enrollments e "
        "ON e.student_id = s.user_id "
        "AND e.course_id = c.course_id "
        "WHERE a.assignment_id = ? "
        "AND l.user_id = ? "
        "AND l.role = 'Lecturer' "
        "AND s.role = 'Student' "
        "AND date(?) > date(a.due_date) "
        "LIMIT 1;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            db,
            authorizationSql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK) {
        return false;
    }

    int result = sqlite3_bind_int(statement, 1, studentId);

    if (result == SQLITE_OK) {
        result = sqlite3_bind_int(statement, 2, assignmentId);
    }

    if (result == SQLITE_OK) {
        result = sqlite3_bind_int(statement, 3, lecturerId);
    }

    if (result == SQLITE_OK) {
        result = sqlite3_bind_text(
            statement,
            4,
            newDueDate.c_str(),
            -1,
            SQLITE_TRANSIENT
        );
    }

    bool authorized = result == SQLITE_OK &&
                      sqlite3_step(statement) == SQLITE_ROW;

    sqlite3_finalize(statement);

    if (!authorized) {
        return false;
    }

    // The schema allows one extension per assignment/student.
    // If one already exists, update it rather than creating
    // a duplicate row.
    const char* insertSql =
        "INSERT INTO extensions "
        "(assignment_id, student_id, new_due_date, approved_by, reason) "
        "VALUES (?, ?, ?, ?, ?) "
        "ON CONFLICT(assignment_id, student_id) DO UPDATE SET "
        "new_due_date = excluded.new_due_date, "
        "approved_by = excluded.approved_by, "
        "reason = excluded.reason;";

    if (sqlite3_prepare_v2(
            db,
            insertSql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK) {
        return false;
    }

    result = sqlite3_bind_int(statement, 1, assignmentId);

    if (result == SQLITE_OK) {
        result = sqlite3_bind_int(statement, 2, studentId);
    }

    if (result == SQLITE_OK) {
        result = sqlite3_bind_text(
            statement,
            3,
            newDueDate.c_str(),
            -1,
            SQLITE_TRANSIENT
        );
    }

    if (result == SQLITE_OK) {
        result = sqlite3_bind_int(statement, 4, lecturerId);
    }

    if (result == SQLITE_OK) {
        result = sqlite3_bind_text(
            statement,
            5,
            reason.c_str(),
            -1,
            SQLITE_TRANSIENT
        );
    }

    bool success = result == SQLITE_OK &&
                   sqlite3_step(statement) == SQLITE_DONE;

    sqlite3_finalize(statement);

    return success;
}
