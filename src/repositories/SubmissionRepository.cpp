
#include "repositories/SubmissionRepository.h"

#include <sqlite3.h>

#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

// Converts a database status string into the corresponding enum.
SubmissionStatus convertStatus(const std::string& status) {
    if (status == "Graded") {
        return SubmissionStatus::Graded;
    }

    if (status == "Moderated") {
        return SubmissionStatus::Moderated;
    }

    if (status == "Released") {
        return SubmissionStatus::Released;
    }

    return SubmissionStatus::Submitted;
}

// Safely reads a text column, including SQL NULL values.
std::string readTextColumn(sqlite3_stmt* statement, int column) {
    const unsigned char* value =
        sqlite3_column_text(statement, column);

    return value == nullptr
        ? ""
        : reinterpret_cast<const char*>(value);
}

// Converts a database row into a Submission object.
Submission readSubmission(sqlite3_stmt* statement) {
    Submission submission(
        sqlite3_column_int(statement, 0),
        sqlite3_column_int(statement, 1),
        sqlite3_column_int(statement, 2),
        readTextColumn(statement, 3),
        readTextColumn(statement, 4)
    );

    if (sqlite3_column_type(statement, 5) != SQLITE_NULL) {
        submission.setMark(sqlite3_column_double(statement, 5));
    }

    submission.setFeedback(readTextColumn(statement, 6));
    submission.setStatus(
        convertStatus(readTextColumn(statement, 7))
    );

    return submission;
}

// Executes SQL statements that do not return rows.
bool executeSql(sqlite3* db, const char* sql) {
    return sqlite3_exec(
        db, sql, nullptr, nullptr, nullptr
    ) == SQLITE_OK;
}

// Rolls back an active transaction.
void rollbackTransaction(sqlite3* db) {
    sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
}

// A reason must contain at least one non-whitespace character.
bool hasValidReason(const std::string& reason) {
    return reason.find_first_not_of(" \t\r\n") !=
           std::string::npos;
}

// Returns the current local timestamp.
std::string currentTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm localTime{};

#ifdef _WIN32
    if (localtime_s(&localTime, &now) != 0) {
        throw std::runtime_error(
            "Failed to obtain current time."
        );
    }
#else
    if (localtime_r(&now, &localTime) == nullptr) {
        throw std::runtime_error(
            "Failed to obtain current time."
        );
    }
#endif

    char buffer[20];

    if (std::strftime(
            buffer,
            sizeof(buffer),
            "%Y-%m-%d %H:%M:%S",
            &localTime
        ) == 0) {
        throw std::runtime_error(
            "Failed to format timestamp."
        );
    }

    return buffer;
}

// Checks whether a user has the specified role.
bool userHasRole(
    sqlite3* db,
    int userId,
    const std::string& role
) {
    const char* sql =
        "SELECT 1 FROM users "
        "WHERE user_id = ? AND role = ? LIMIT 1;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            db, sql, -1, &statement, nullptr
        ) != SQLITE_OK) {
        return false;
    }

    int result = sqlite3_bind_int(statement, 1, userId);

    if (result == SQLITE_OK) {
        result = sqlite3_bind_text(
            statement,
            2,
            role.c_str(),
            -1,
            SQLITE_TRANSIENT
        );
    }

    bool valid = result == SQLITE_OK &&
                 sqlite3_step(statement) == SQLITE_ROW;

    sqlite3_finalize(statement);
    return valid;
}

// Confirms that a student is enrolled in the assignment's course.
bool studentEnrolledInAssignment(
    sqlite3* db,
    int studentId,
    int assignmentId
) {
    const char* sql =
        "SELECT 1 "
        "FROM users u "
        "JOIN enrollments e ON e.student_id = u.user_id "
        "JOIN assignments a ON a.course_id = e.course_id "
        "WHERE u.user_id = ? "
        "AND u.role = 'Student' "
        "AND a.assignment_id = ? "
        "LIMIT 1;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            db, sql, -1, &statement, nullptr
        ) != SQLITE_OK) {
        return false;
    }

    int result = sqlite3_bind_int(statement, 1, studentId);

    if (result == SQLITE_OK) {
        result = sqlite3_bind_int(statement, 2, assignmentId);
    }

    bool enrolled = result == SQLITE_OK &&
                    sqlite3_step(statement) == SQLITE_ROW;

    sqlite3_finalize(statement);
    return enrolled;
}

// Confirms the user's role, department, and relationship
// to the submission being accessed.
bool isAuthorizedForSubmission(
    sqlite3* db,
    int userId,
    int submissionId,
    const std::string& requiredRole
) {
    const char* sql =
        "SELECT 1 "
        "FROM users u "
        "JOIN courses c "
        "ON c.department_id = u.department_id "
        "JOIN assignments a "
        "ON a.course_id = c.course_id "
        "JOIN submissions s "
        "ON s.assignment_id = a.assignment_id "
        "WHERE u.user_id = ? "
        "AND s.submission_id = ? "
        "AND u.role = ? "
        "LIMIT 1;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            db, sql, -1, &statement, nullptr
        ) != SQLITE_OK) {
        return false;
    }

    int result = sqlite3_bind_int(statement, 1, userId);

    if (result == SQLITE_OK) {
        result = sqlite3_bind_int(statement, 2, submissionId);
    }

    if (result == SQLITE_OK) {
        result = sqlite3_bind_text(
            statement,
            3,
            requiredRole.c_str(),
            -1,
            SQLITE_TRANSIENT
        );
    }

    bool authorized = result == SQLITE_OK &&
                      sqlite3_step(statement) == SQLITE_ROW;

    sqlite3_finalize(statement);
    return authorized;
}

// Checks whether a mark is within the assignment's maximum.
bool isMarkWithinLimit(
    sqlite3* db,
    int submissionId,
    double mark
) {
    const char* sql =
        "SELECT a.max_mark "
        "FROM submissions s "
        "JOIN assignments a "
        "ON a.assignment_id = s.assignment_id "
        "WHERE s.submission_id = ?;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            db, sql, -1, &statement, nullptr
        ) != SQLITE_OK) {
        return false;
    }

    int result = sqlite3_bind_int(statement, 1, submissionId);
    bool valid = false;

    if (result == SQLITE_OK &&
        sqlite3_step(statement) == SQLITE_ROW) {
        double maximumMark =
            sqlite3_column_double(statement, 0);

        valid = std::isfinite(maximumMark) &&
                maximumMark > 0.0 &&
                mark >= 0.0 &&
                mark <= maximumMark;
    }

    sqlite3_finalize(statement);
    return valid;
}

// Retrieves the existing mark, if any.
bool getOldMark(
    sqlite3* db,
    int submissionId,
    bool& hasOldMark,
    double& oldMark
) {
    const char* sql =
        "SELECT mark FROM submissions "
        "WHERE submission_id = ?;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            db, sql, -1, &statement, nullptr
        ) != SQLITE_OK) {
        return false;
    }

    int result = sqlite3_bind_int(statement, 1, submissionId);
    bool found = false;

    if (result == SQLITE_OK &&
        sqlite3_step(statement) == SQLITE_ROW) {
        found = true;

        hasOldMark =
            sqlite3_column_type(statement, 0) != SQLITE_NULL;

        if (hasOldMark) {
            oldMark = sqlite3_column_double(statement, 0);
        }
    }

    sqlite3_finalize(statement);
    return found;
}

// Parses YYYY-MM-DD and validates that it is a real calendar date.
bool parseDate(
    const std::string& value,
    std::tm& parsedDate
) {
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

    // Noon avoids most daylight-saving boundary problems.
    date.tm_hour = 12;
    date.tm_min = 0;
    date.tm_sec = 0;
    date.tm_isdst = -1;

    int originalYear = date.tm_year;
    int originalMonth = date.tm_mon;
    int originalDay = date.tm_mday;

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

    return normalized.tm_year == originalYear &&
           normalized.tm_mon == originalMonth &&
           normalized.tm_mday == originalDay;
}

// Calculates calendar days late, ignoring the time of day.
// Deadline format: YYYY-MM-DD.
// Submission format: YYYY-MM-DD HH:MM:SS.
bool calculateCalendarDaysLate(
    const std::string& submittedAt,
    const std::string& dueDate,
    int& daysLate
) {
    if (submittedAt.size() < 10) {
        return false;
    }

    std::tm submissionDate{};
    std::tm deadlineDate{};

    if (!parseDate(submittedAt.substr(0, 10), submissionDate) ||
        !parseDate(dueDate, deadlineDate)) {
        return false;
    }

    std::time_t submittedTime =
        std::mktime(&submissionDate);

    std::time_t deadlineTime =
        std::mktime(&deadlineDate);

    if (submittedTime == static_cast<std::time_t>(-1) ||
        deadlineTime == static_cast<std::time_t>(-1)) {
        return false;
    }

    double difference =
        std::difftime(submittedTime, deadlineTime) / 86400.0;

    // Both timestamps are normalized to noon on their dates.
    // Each calendar date after the deadline adds one late day.
    if (difference <= 0.0) {
        daysLate = 0;
    } else {
        daysLate = static_cast<int>(difference + 0.5);
    }

    return true;
}

// Retrieves the submission timestamp, applicable deadline,
// and maximum mark. Individual extensions take precedence.
bool getGradingDetails(
    sqlite3* db,
    int submissionId,
    std::string& submittedAt,
    std::string& applicableDeadline,
    double& maximumMark
) {
    const char* sql =
        "SELECT "
        "s.submitted_at, "
        "COALESCE(e.new_due_date, a.due_date), "
        "a.max_mark "
        "FROM submissions s "
        "JOIN assignments a "
        "ON a.assignment_id = s.assignment_id "
        "LEFT JOIN extensions e "
        "ON e.assignment_id = s.assignment_id "
        "AND e.student_id = s.student_id "
        "WHERE s.submission_id = ?;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            db, sql, -1, &statement, nullptr
        ) != SQLITE_OK) {
        return false;
    }

    int result = sqlite3_bind_int(statement, 1, submissionId);

    if (result != SQLITE_OK) {
        sqlite3_finalize(statement);
        return false;
    }

    result = sqlite3_step(statement);

    if (result != SQLITE_ROW) {
        sqlite3_finalize(statement);
        return false;
    }

    submittedAt = readTextColumn(statement, 0);
    applicableDeadline = readTextColumn(statement, 1);
    maximumMark = sqlite3_column_double(statement, 2);

    sqlite3_finalize(statement);

    return !submittedAt.empty() &&
           !applicableDeadline.empty() &&
           std::isfinite(maximumMark) &&
           maximumMark > 0.0;
}

// Records the grading action and its final mark in the audit log.
bool insertGradingAudit(
    sqlite3* db,
    int submissionId,
    int lecturerId,
    bool hasOldMark,
    double oldMark,
    double newMark,
    const std::string& reason
) {
    const char* sql =
        "INSERT INTO audit_logs "
        "(submission_id, changed_by, changed_at, old_mark, "
        "new_mark, reason) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            db, sql, -1, &statement, nullptr
        ) != SQLITE_OK) {
        return false;
    }

    std::string timestamp;

    try {
        timestamp = currentTimestamp();
    } catch (...) {
        sqlite3_finalize(statement);
        return false;
    }

    int result = sqlite3_bind_int(statement, 1, submissionId);

    if (result == SQLITE_OK) {
        result = sqlite3_bind_int(statement, 2, lecturerId);
    }

    if (result == SQLITE_OK) {
        result = sqlite3_bind_text(
            statement, 3, timestamp.c_str(),
            -1, SQLITE_TRANSIENT
        );
    }

    if (result == SQLITE_OK) {
        result = hasOldMark
            ? sqlite3_bind_double(statement, 4, oldMark)
            : sqlite3_bind_null(statement, 4);
    }

    if (result == SQLITE_OK) {
        result = sqlite3_bind_double(statement, 5, newMark);
    }

    if (result == SQLITE_OK) {
        result = sqlite3_bind_text(
            statement, 6, reason.c_str(),
            -1, SQLITE_TRANSIENT
        );
    }

    bool success = result == SQLITE_OK &&
                   sqlite3_step(statement) == SQLITE_DONE;

    sqlite3_finalize(statement);
    return success;
}

} // namespace


SubmissionRepository::SubmissionRepository(Database& database)
    : database(database) {
}


// Creates a submission for a valid, enrolled student.
bool SubmissionRepository::addSubmission(
    const Submission& submission
) {
    sqlite3* db = database.getConnection();

    if (db == nullptr ||
        submission.getStudentId() <= 0 ||
        submission.getAssignmentId() <= 0 ||
        submission.getSubmittedAt().empty() ||
        submission.getFilePath().empty()) {
        return false;
    }

    if (!studentEnrolledInAssignment(
            db,
            submission.getStudentId(),
            submission.getAssignmentId()
        )) {
        return false;
    }

    const char* sql =
        "INSERT INTO submissions "
        "(student_id, assignment_id, submitted_at, file_path, "
        "mark, feedback, status) "
        "VALUES (?, ?, ?, ?, NULL, '', 'Submitted');";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            db, sql, -1, &statement, nullptr
        ) != SQLITE_OK) {
        return false;
    }

    int result = sqlite3_bind_int(
        statement, 1, submission.getStudentId()
    );

    if (result == SQLITE_OK) {
        result = sqlite3_bind_int(
            statement, 2, submission.getAssignmentId()
        );
    }

    if (result == SQLITE_OK) {
        result = sqlite3_bind_text(
            statement, 3,
            submission.getSubmittedAt().c_str(),
            -1, SQLITE_TRANSIENT
        );
    }

    if (result == SQLITE_OK) {
        result = sqlite3_bind_text(
            statement, 4,
            submission.getFilePath().c_str(),
            -1, SQLITE_TRANSIENT
        );
    }

    bool success = result == SQLITE_OK &&
                   sqlite3_step(statement) == SQLITE_DONE;

    sqlite3_finalize(statement);
    return success;
}


// Retrieves submissions for a student ID.
// The caller must authenticate the student separately.
std::vector<Submission>
SubmissionRepository::getSubmissionsByStudent(int studentId) {
    std::vector<Submission> submissions;
    sqlite3* db = database.getConnection();

    if (db == nullptr || studentId <= 0) {
        return submissions;
    }

    const char* sql =
        "SELECT submission_id, student_id, assignment_id, "
        "submitted_at, file_path, mark, feedback, status "
        "FROM submissions "
        "WHERE student_id = ? "
        "ORDER BY submitted_at DESC;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            db, sql, -1, &statement, nullptr
        ) != SQLITE_OK) {
        throw std::runtime_error(
            "Failed to retrieve student submissions."
        );
    }

    int bindResult = sqlite3_bind_int(statement, 1, studentId);

    if (bindResult != SQLITE_OK) {
        sqlite3_finalize(statement);
        throw std::runtime_error(
            "Failed to bind student ID."
        );
    }

    int result;

    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        submissions.push_back(readSubmission(statement));
    }

    sqlite3_finalize(statement);

    if (result != SQLITE_DONE) {
        throw std::runtime_error(
            "Error while reading student submissions."
        );
    }

    return submissions;
}


// Retrieves submissions belonging to the authenticated student.
std::vector<Submission>
SubmissionRepository::getSubmissionsForAuthenticatedStudent(
    int studentId,
    int authenticatedUserId
) {
    std::vector<Submission> submissions;

    if (studentId <= 0 ||
        authenticatedUserId <= 0 ||
        studentId != authenticatedUserId) {
        return submissions;
    }

    sqlite3* db = database.getConnection();

    if (db == nullptr) {
        return submissions;
    }

    const char* sql =
        "SELECT s.submission_id, s.student_id, s.assignment_id, "
        "s.submitted_at, s.file_path, s.mark, s.feedback, s.status "
        "FROM submissions s "
        "JOIN users u ON u.user_id = s.student_id "
        "WHERE s.student_id = ? "
        "AND u.user_id = ? "
        "AND u.role = 'Student' "
        "ORDER BY s.submitted_at DESC;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            db, sql, -1, &statement, nullptr
        ) != SQLITE_OK) {
        throw std::runtime_error(
            "Failed to retrieve authenticated student submissions."
        );
    }

    int result = sqlite3_bind_int(statement, 1, studentId);

    if (result == SQLITE_OK) {
        result = sqlite3_bind_int(
            statement, 2, authenticatedUserId
        );
    }

    if (result != SQLITE_OK) {
        sqlite3_finalize(statement);
        throw std::runtime_error(
            "Failed to bind authenticated student IDs."
        );
    }

    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        submissions.push_back(readSubmission(statement));
    }

    sqlite3_finalize(statement);

    if (result != SQLITE_DONE) {
        throw std::runtime_error(
            "Error while reading authenticated student submissions."
        );
    }

    return submissions;
}


// Retrieves submissions for a department.
std::vector<Submission>
SubmissionRepository::getSubmissionsByDepartment(int departmentId) {
    std::vector<Submission> submissions;
    sqlite3* db = database.getConnection();

    if (db == nullptr || departmentId <= 0) {
        return submissions;
    }

    const char* sql =
        "SELECT s.submission_id, s.student_id, s.assignment_id, "
        "s.submitted_at, s.file_path, s.mark, s.feedback, s.status "
        "FROM submissions s "
        "JOIN assignments a ON a.assignment_id = s.assignment_id "
        "JOIN courses c ON c.course_id = a.course_id "
        "WHERE c.department_id = ? "
        "ORDER BY s.submitted_at DESC;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            db, sql, -1, &statement, nullptr
        ) != SQLITE_OK) {
        throw std::runtime_error(
            "Failed to retrieve department submissions."
        );
    }

    int bindResult = sqlite3_bind_int(
        statement, 1, departmentId
    );

    if (bindResult != SQLITE_OK) {
        sqlite3_finalize(statement);
        throw std::runtime_error(
            "Failed to bind department ID."
        );
    }

    int result;

    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        submissions.push_back(readSubmission(statement));
    }

    sqlite3_finalize(statement);

    if (result != SQLITE_DONE) {
        throw std::runtime_error(
            "Error while reading department submissions."
        );
    }

    return submissions;
}


// Retrieves all submissions.
std::vector<Submission>
SubmissionRepository::getAllSubmissions() {
    std::vector<Submission> submissions;
    sqlite3* db = database.getConnection();

    if (db == nullptr) {
        return submissions;
    }

    const char* sql =
        "SELECT submission_id, student_id, assignment_id, "
        "submitted_at, file_path, mark, feedback, status "
        "FROM submissions "
        "ORDER BY submitted_at DESC;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            db, sql, -1, &statement, nullptr
        ) != SQLITE_OK) {
        throw std::runtime_error(
            "Failed to retrieve all submissions."
        );
    }

    int result;

    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        submissions.push_back(readSubmission(statement));
    }

    sqlite3_finalize(statement);

    if (result != SQLITE_DONE) {
        throw std::runtime_error(
            "Error while reading all submissions."
        );
    }

    return submissions;
}


// Grades a submission, applies the late penalty, and records
// the final adjusted mark in the audit log.
bool SubmissionRepository::gradeSubmission(
    int submissionId,
    double mark,
    const std::string& feedback,
    int lecturerId,
    const std::string& reason
) {
    sqlite3* db = database.getConnection();

    if (db == nullptr ||
        submissionId <= 0 ||
        lecturerId <= 0 ||
        !std::isfinite(mark) ||
        mark < 0.0 ||
        !hasValidReason(reason)) {
        return false;
    }

    if (!executeSql(db, "BEGIN TRANSACTION;")) {
        return false;
    }

    // Only a lecturer in the assignment's department may grade it.
    if (!isAuthorizedForSubmission(
            db, lecturerId, submissionId, "Lecturer"
        ) ||
        !userHasRole(db, lecturerId, "Lecturer") ||
        !isMarkWithinLimit(db, submissionId, mark)) {
        rollbackTransaction(db);
        return false;
    }

    bool hasOldMark = false;
    double oldMark = 0.0;

    if (!getOldMark(
            db, submissionId, hasOldMark, oldMark
        )) {
        rollbackTransaction(db);
        return false;
    }

    // Retrieve the submitted timestamp, applicable deadline,
    // and maximum mark from the database.
    std::string submittedAt;
    std::string applicableDeadline;
    double maximumMark = 0.0;

    if (!getGradingDetails(
            db,
            submissionId,
            submittedAt,
            applicableDeadline,
            maximumMark
        )) {
        rollbackTransaction(db);
        return false;
    }

    // Calculate lateness by calendar date.
    int daysLate = 0;

    if (!calculateCalendarDaysLate(
            submittedAt,
            applicableDeadline,
            daysLate
        )) {
        rollbackTransaction(db);
        return false;
    }

    // Apply the existing three-day graduated late-penalty policy.
    ThreeDayGraduatedLatePenaltyPolicy penaltyPolicy;

    double finalMark = 0.0;

    try {
        finalMark = penaltyPolicy.calculateFinalMark(
            mark,
            maximumMark,
            daysLate
        );
    } catch (...) {
        rollbackTransaction(db);
        return false;
    }

    if (!std::isfinite(finalMark) ||
        finalMark < 0.0 ||
        finalMark > maximumMark) {
        rollbackTransaction(db);
        return false;
    }

    // Store the adjusted mark, not the original awarded mark.
    const char* updateSql =
        "UPDATE submissions "
        "SET mark = ?, feedback = ?, status = 'Graded' "
        "WHERE submission_id = ? "
        "AND status IN ('Submitted', 'Graded');";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            db, updateSql, -1, &statement, nullptr
        ) != SQLITE_OK) {
        rollbackTransaction(db);
        return false;
    }

    int result = sqlite3_bind_double(statement, 1, finalMark);

    if (result == SQLITE_OK) {
        result = sqlite3_bind_text(
            statement, 2,
            feedback.c_str(),
            -1, SQLITE_TRANSIENT
        );
    }

    if (result == SQLITE_OK) {
        result = sqlite3_bind_int(
            statement, 3, submissionId
        );
    }

    bool success = result == SQLITE_OK &&
                   sqlite3_step(statement) == SQLITE_DONE &&
                   sqlite3_changes(db) == 1;

    sqlite3_finalize(statement);

    // The audit record must contain the same final mark
    // that is stored in the submission.
    if (!success ||
        !insertGradingAudit(
            db,
            submissionId,
            lecturerId,
            hasOldMark,
            oldMark,
            finalMark,
            reason
        )) {
        rollbackTransaction(db);
        return false;
    }

    if (!executeSql(db, "COMMIT;")) {
        rollbackTransaction(db);
        return false;
    }

    return true;
}


// Only the relevant department's HOD can moderate a graded submission.
// Moderation does not recalculate the mark.
bool SubmissionRepository::moderateSubmission(
    int submissionId,
    int hodId,
    const std::string& reason
) {
    sqlite3* db = database.getConnection();

    if (db == nullptr ||
        submissionId <= 0 ||
        hodId <= 0 ||
        !hasValidReason(reason)) {
        return false;
    }

    if (!executeSql(db, "BEGIN TRANSACTION;")) {
        return false;
    }

    if (!isAuthorizedForSubmission(
            db, hodId, submissionId, "HeadOfDepartment"
        ) ||
        !userHasRole(db, hodId, "HeadOfDepartment")) {
        rollbackTransaction(db);
        return false;
    }

    const char* sql =
        "UPDATE submissions "
        "SET status = 'Moderated' "
        "WHERE submission_id = ? "
        "AND status = 'Graded';";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            db, sql, -1, &statement, nullptr
        ) != SQLITE_OK) {
        rollbackTransaction(db);
        return false;
    }

    int result = sqlite3_bind_int(statement, 1, submissionId);

    bool success = result == SQLITE_OK &&
                   sqlite3_step(statement) == SQLITE_DONE &&
                   sqlite3_changes(db) == 1;

    sqlite3_finalize(statement);

    if (!success || !executeSql(db, "COMMIT;")) {
        rollbackTransaction(db);
        return false;
    }

    return true;
}


// Only the relevant department's HOD can release a moderated submission.
// Releasing does not recalculate the mark.
bool SubmissionRepository::releaseSubmission(
    int submissionId,
    int hodId,
    const std::string& reason
) {
    sqlite3* db = database.getConnection();

    if (db == nullptr ||
        submissionId <= 0 ||
        hodId <= 0 ||
        !hasValidReason(reason)) {
        return false;
    }

    if (!executeSql(db, "BEGIN TRANSACTION;")) {
        return false;
    }

    if (!isAuthorizedForSubmission(
            db, hodId, submissionId, "HeadOfDepartment"
        ) ||
        !userHasRole(db, hodId, "HeadOfDepartment")) {
        rollbackTransaction(db);
        return false;
    }

    const char* sql =
        "UPDATE submissions "
        "SET status = 'Released' "
        "WHERE submission_id = ? "
        "AND status = 'Moderated';";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            db, sql, -1, &statement, nullptr
        ) != SQLITE_OK) {
        rollbackTransaction(db);
        return false;
    }

    int result = sqlite3_bind_int(statement, 1, submissionId);

    bool success = result == SQLITE_OK &&
                   sqlite3_step(statement) == SQLITE_DONE &&
                   sqlite3_changes(db) == 1;

    sqlite3_finalize(statement);

    if (!success || !executeSql(db, "COMMIT;")) {
        rollbackTransaction(db);
        return false;
    }

    return true;
}
