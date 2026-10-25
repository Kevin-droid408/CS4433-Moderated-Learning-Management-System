
#include "repositories/SubmissionRepository.h"

#include <sqlite3.h>

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

namespace
{
    // Converts a database status into the SubmissionStatus enum.
    SubmissionStatus convertStatus(const string& status)
    {
        if (status == "Graded")
            return SubmissionStatus::Graded;

        if (status == "Moderated")
            return SubmissionStatus::Moderated;

        if (status == "Released")
            return SubmissionStatus::Released;

        return SubmissionStatus::Submitted;
    }

    // Safely reads a text column from an SQLite result.
    string readTextColumn(sqlite3_stmt* statement, int column)
    {
        const unsigned char* value =
            sqlite3_column_text(statement, column);

        if (value == nullptr)
            return "";

        return reinterpret_cast<const char*>(value);
    }

    // Converts a database row into a Submission object.
    Submission readSubmission(sqlite3_stmt* statement)
    {
        Submission submission(
            sqlite3_column_int(statement, 0),
            sqlite3_column_int(statement, 1),
            sqlite3_column_int(statement, 2),
            readTextColumn(statement, 3),
            readTextColumn(statement, 4)
        );

        if (sqlite3_column_type(statement, 5) != SQLITE_NULL)
        {
            submission.setMark(
                sqlite3_column_double(statement, 5)
            );
        }

        submission.setFeedback(readTextColumn(statement, 6));

        submission.setStatus(
            convertStatus(readTextColumn(statement, 7))
        );

        return submission;
    }

    // Executes a transaction-control statement.
    void executeSql(sqlite3* connection, const char* sql)
    {
        char* errorMessage = nullptr;

        int result = sqlite3_exec(
            connection,
            sql,
            nullptr,
            nullptr,
            &errorMessage
        );

        if (result != SQLITE_OK)
        {
            string message = errorMessage != nullptr
                ? errorMessage
                : sqlite3_errmsg(connection);

            sqlite3_free(errorMessage);

            throw runtime_error(message);
        }
    }

    // Rolls back without replacing the original error.
    void rollbackTransaction(sqlite3* connection)
    {
        sqlite3_exec(
            connection,
            "ROLLBACK;",
            nullptr,
            nullptr,
            nullptr
        );
    }

    // Checks whether a required reason contains non-whitespace text.
    bool hasValidReason(const string& reason)
    {
        return reason.find_first_not_of(" \t\n\r") != string::npos;
    }

    /*
        Checks whether the supplied user has the required role
        and belongs to the department responsible for the submission.

        role must be "Lecturer" or "HeadOfDepartment".
    */
    bool isAuthorizedForSubmission(
        sqlite3* connection,
        int submissionId,
        int userId,
        const string& role)
    {
        const char* sql =
            "SELECT EXISTS ("
            " SELECT 1 "
            " FROM users AS u "
            " JOIN submissions AS s "
            "   ON s.submission_id = ? "
            " JOIN assignments AS a "
            "   ON a.assignment_id = s.assignment_id "
            " JOIN courses AS c "
            "   ON c.course_id = a.course_id "
            " WHERE u.user_id = ? "
            "   AND u.role = ? "
            "   AND u.department_id = c.department_id"
            ");";

        sqlite3_stmt* statement = nullptr;

        int result = sqlite3_prepare_v2(
            connection,
            sql,
            -1,
            &statement,
            nullptr
        );

        if (result != SQLITE_OK)
        {
            throw runtime_error(
                string("Failed to prepare authorization check: ") +
                sqlite3_errmsg(connection)
            );
        }

        sqlite3_bind_int(statement, 1, submissionId);
        sqlite3_bind_int(statement, 2, userId);

        sqlite3_bind_text(
            statement,
            3,
            role.c_str(),
            -1,
            SQLITE_TRANSIENT
        );

        result = sqlite3_step(statement);

        if (result != SQLITE_ROW)
        {
            string error = sqlite3_errmsg(connection);
            sqlite3_finalize(statement);

            throw runtime_error(
                "Failed to check user authorization: " + error
            );
        }

        bool authorized = sqlite3_column_int(statement, 0) == 1;

        sqlite3_finalize(statement);

        return authorized;
    }

    /*
        Changes a submission's status and writes an audit record.

        The caller must have started a transaction and verified
        authorization and the expected current status.
    */
    void updateStatusAndAudit(
        sqlite3* connection,
        int submissionId,
        int actorId,
        const string& expectedStatus,
        const string& newStatus,
        const string& reason,
        double currentMark)
    {
        const char* updateSql =
            "UPDATE submissions "
            "SET status = ? "
            "WHERE submission_id = ? AND status = ?;";

        sqlite3_stmt* updateStatement = nullptr;

        int result = sqlite3_prepare_v2(
            connection,
            updateSql,
            -1,
            &updateStatement,
            nullptr
        );

        if (result != SQLITE_OK)
        {
            throw runtime_error(
                string("Failed to prepare status update: ") +
                sqlite3_errmsg(connection)
            );
        }

        sqlite3_bind_text(
            updateStatement,
            1,
            newStatus.c_str(),
            -1,
            SQLITE_TRANSIENT
        );

        sqlite3_bind_int(updateStatement, 2, submissionId);

        sqlite3_bind_text(
            updateStatement,
            3,
            expectedStatus.c_str(),
            -1,
            SQLITE_TRANSIENT
        );

        result = sqlite3_step(updateStatement);
        string updateError = sqlite3_errmsg(connection);

        if (result != SQLITE_DONE)
        {
            sqlite3_finalize(updateStatement);

            throw runtime_error(
                "Failed to update submission status: " + updateError
            );
        }

        int rowsChanged = sqlite3_changes(connection);

        sqlite3_finalize(updateStatement);

        if (rowsChanged != 1)
        {
            throw runtime_error(
                "Submission status changed unexpectedly."
            );
        }

        // Record the moderation or release action.
        const char* auditSql =
            "INSERT INTO audit_logs "
            "(submission_id, changed_by, changed_at, "
            "old_mark, new_mark, reason) "
            "VALUES (?, ?, datetime('now', 'localtime'), ?, ?, ?);";

        sqlite3_stmt* auditStatement = nullptr;

        result = sqlite3_prepare_v2(
            connection,
            auditSql,
            -1,
            &auditStatement,
            nullptr
        );

        if (result != SQLITE_OK)
        {
            throw runtime_error(
                string("Failed to prepare audit log: ") +
                sqlite3_errmsg(connection)
            );
        }

        sqlite3_bind_int(auditStatement, 1, submissionId);
        sqlite3_bind_int(auditStatement, 2, actorId);
        sqlite3_bind_double(auditStatement, 3, currentMark);
        sqlite3_bind_double(auditStatement, 4, currentMark);

        sqlite3_bind_text(
            auditStatement,
            5,
            reason.c_str(),
            -1,
            SQLITE_TRANSIENT
        );

        result = sqlite3_step(auditStatement);
        string auditError = sqlite3_errmsg(connection);

        sqlite3_finalize(auditStatement);

        if (result != SQLITE_DONE)
        {
            throw runtime_error(
                "Failed to record audit action: " + auditError
            );
        }
    }
}

// Constructor
SubmissionRepository::SubmissionRepository(Database& database)
    : database(database)
{
}

// Adds a new student submission.
bool SubmissionRepository::addSubmission(
    const Submission& submission)
{
    const char* sql =
        "INSERT INTO submissions "
        "(student_id, assignment_id, submitted_at, file_path, "
        "mark, feedback, status) "
        "VALUES (?, ?, ?, ?, NULL, '', 'Submitted');";

    sqlite3* connection = database.getConnection();
    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        connection, sql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK)
    {
        throw runtime_error(
            string("Failed to prepare submission insertion: ") +
            sqlite3_errmsg(connection)
        );
    }

    sqlite3_bind_int(statement, 1, submission.getStudentId());
    sqlite3_bind_int(statement, 2, submission.getAssignmentId());

    sqlite3_bind_text(
        statement,
        3,
        submission.getSubmittedAt().c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement,
        4,
        submission.getFilePath().c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    result = sqlite3_step(statement);
    string errorMessage;

    if (result != SQLITE_DONE)
        errorMessage = sqlite3_errmsg(connection);

    sqlite3_finalize(statement);

    if (result == SQLITE_DONE)
        return true;

    if ((result & 0xFF) == SQLITE_CONSTRAINT)
        return false;

    throw runtime_error(
        "Failed to save submission: " + errorMessage
    );
}

// Retrieves submissions belonging to one student.
vector<Submission>
SubmissionRepository::getSubmissionsByStudent(int studentId)
{
    vector<Submission> submissions;

    const char* sql =
        "SELECT submission_id, student_id, assignment_id, "
        "submitted_at, file_path, mark, feedback, status "
        "FROM submissions "
        "WHERE student_id = ? "
        "ORDER BY submitted_at DESC;";

    sqlite3* connection = database.getConnection();
    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        connection, sql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK)
    {
        throw runtime_error(
            string("Failed to prepare student submissions query: ") +
            sqlite3_errmsg(connection)
        );
    }

    sqlite3_bind_int(statement, 1, studentId);

    while ((result = sqlite3_step(statement)) == SQLITE_ROW)
        submissions.push_back(readSubmission(statement));

    string errorMessage;

    if (result != SQLITE_DONE)
        errorMessage = sqlite3_errmsg(connection);

    sqlite3_finalize(statement);

    if (result != SQLITE_DONE)
    {
        throw runtime_error(
            "Failed to retrieve student submissions: " + errorMessage
        );
    }

    return submissions;
}

// Retrieves all submissions for staff review.
vector<Submission>
SubmissionRepository::getAllSubmissions()
{
    vector<Submission> submissions;

    const char* sql =
        "SELECT submission_id, student_id, assignment_id, "
        "submitted_at, file_path, mark, feedback, status "
        "FROM submissions "
        "ORDER BY submitted_at DESC;";

    sqlite3* connection = database.getConnection();
    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        connection, sql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK)
    {
        throw runtime_error(
            string("Failed to prepare all submissions query: ") +
            sqlite3_errmsg(connection)
        );
    }

    while ((result = sqlite3_step(statement)) == SQLITE_ROW)
        submissions.push_back(readSubmission(statement));

    string errorMessage;

    if (result != SQLITE_DONE)
        errorMessage = sqlite3_errmsg(connection);

    sqlite3_finalize(statement);

    if (result != SQLITE_DONE)
    {
        throw runtime_error(
            "Failed to retrieve all submissions: " + errorMessage
        );
    }

    return submissions;
}

// Grades a submission and records the mark change.
bool SubmissionRepository::gradeSubmission(
    int submissionId,
    double mark,
    const string& feedback,
    int lecturerId,
    const string& reason)
{
    if (submissionId <= 0 ||
        lecturerId <= 0 ||
        !isfinite(mark) ||
        mark < 0.0 ||
        !hasValidReason(reason))
    {
        return false;
    }

    sqlite3* connection = database.getConnection();

    try
    {
        executeSql(connection, "BEGIN IMMEDIATE;");

        // Only lecturers in the course's department may grade.
        if (!isAuthorizedForSubmission(
                connection,
                submissionId,
                lecturerId,
                "Lecturer"))
        {
            rollbackTransaction(connection);
            return false;
        }

        const char* selectSql =
            "SELECT s.mark, a.max_mark, s.status "
            "FROM submissions AS s "
            "JOIN assignments AS a "
            "ON a.assignment_id = s.assignment_id "
            "WHERE s.submission_id = ?;";

        sqlite3_stmt* selectStatement = nullptr;

        int result = sqlite3_prepare_v2(
            connection,
            selectSql,
            -1,
            &selectStatement,
            nullptr
        );

        if (result != SQLITE_OK)
        {
            throw runtime_error(
                string("Failed to prepare grading query: ") +
                sqlite3_errmsg(connection)
            );
        }

        sqlite3_bind_int(selectStatement, 1, submissionId);
        result = sqlite3_step(selectStatement);

        if (result != SQLITE_ROW)
        {
            string errorMessage = sqlite3_errmsg(connection);
            sqlite3_finalize(selectStatement);

            if (result == SQLITE_DONE)
            {
                rollbackTransaction(connection);
                return false;
            }

            throw runtime_error(
                "Failed to retrieve submission for grading: " +
                errorMessage
            );
        }

        bool hasOldMark =
            sqlite3_column_type(selectStatement, 0) != SQLITE_NULL;

        double oldMark = hasOldMark
            ? sqlite3_column_double(selectStatement, 0)
            : 0.0;

        double maximumMark =
            sqlite3_column_double(selectStatement, 1);

        string status = readTextColumn(selectStatement, 2);

        sqlite3_finalize(selectStatement);

        if (!isfinite(maximumMark) ||
            maximumMark <= 0.0 ||
            mark > maximumMark ||
            (status != "Submitted" && status != "Graded"))
        {
            rollbackTransaction(connection);
            return false;
        }

        const char* updateSql =
            "UPDATE submissions "
            "SET mark = ?, feedback = ?, status = 'Graded' "
            "WHERE submission_id = ? "
            "AND status IN ('Submitted', 'Graded');";

        sqlite3_stmt* updateStatement = nullptr;

        result = sqlite3_prepare_v2(
            connection,
            updateSql,
            -1,
            &updateStatement,
            nullptr
        );

        if (result != SQLITE_OK)
        {
            throw runtime_error(
                string("Failed to prepare grade update: ") +
                sqlite3_errmsg(connection)
            );
        }

        sqlite3_bind_double(updateStatement, 1, mark);

        sqlite3_bind_text(
            updateStatement,
            2,
            feedback.c_str(),
            -1,
            SQLITE_TRANSIENT
        );

        sqlite3_bind_int(updateStatement, 3, submissionId);

        result = sqlite3_step(updateStatement);
        string updateError = sqlite3_errmsg(connection);

        if (result != SQLITE_DONE)
        {
            sqlite3_finalize(updateStatement);

            throw runtime_error(
                "Failed to update submission grade: " + updateError
            );
        }

        int rowsChanged = sqlite3_changes(connection);
        sqlite3_finalize(updateStatement);

        if (rowsChanged != 1)
        {
            rollbackTransaction(connection);
            return false;
        }

        const char* auditSql =
            "INSERT INTO audit_logs "
            "(submission_id, changed_by, changed_at, "
            "old_mark, new_mark, reason) "
            "VALUES (?, ?, datetime('now', 'localtime'), ?, ?, ?);";

        sqlite3_stmt* auditStatement = nullptr;

        result = sqlite3_prepare_v2(
            connection,
            auditSql,
            -1,
            &auditStatement,
            nullptr
        );

        if (result != SQLITE_OK)
        {
            throw runtime_error(
                string("Failed to prepare grading audit log: ") +
                sqlite3_errmsg(connection)
            );
        }

        sqlite3_bind_int(auditStatement, 1, submissionId);
        sqlite3_bind_int(auditStatement, 2, lecturerId);

        if (hasOldMark)
            sqlite3_bind_double(auditStatement, 3, oldMark);
        else
            sqlite3_bind_null(auditStatement, 3);

        sqlite3_bind_double(auditStatement, 4, mark);

        sqlite3_bind_text(
            auditStatement,
            5,
            reason.c_str(),
            -1,
            SQLITE_TRANSIENT
        );

        result = sqlite3_step(auditStatement);
        string auditError = sqlite3_errmsg(connection);

        sqlite3_finalize(auditStatement);

        if (result != SQLITE_DONE)
        {
            throw runtime_error(
                "Failed to record grade change: " + auditError
            );
        }

        executeSql(connection, "COMMIT;");
        return true;
    }
    catch (...)
    {
        rollbackTransaction(connection);
        throw;
    }
}

// Moderates a graded submission after verifying HOD authorization.
bool SubmissionRepository::moderateSubmission(
    int submissionId,
    int hodId,
    const string& reason)
{
    if (submissionId <= 0 ||
        hodId <= 0 ||
        !hasValidReason(reason))
    {
        return false;
    }

    sqlite3* connection = database.getConnection();

    try
    {
        executeSql(connection, "BEGIN IMMEDIATE;");

        if (!isAuthorizedForSubmission(
                connection,
                submissionId,
                hodId,
                "HeadOfDepartment"))
        {
            rollbackTransaction(connection);
            return false;
        }

        const char* selectSql =
            "SELECT mark, status "
            "FROM submissions "
            "WHERE submission_id = ?;";

        sqlite3_stmt* statement = nullptr;

        int result = sqlite3_prepare_v2(
            connection,
            selectSql,
            -1,
            &statement,
            nullptr
        );

        if (result != SQLITE_OK)
        {
            throw runtime_error(
                string("Failed to prepare moderation query: ") +
                sqlite3_errmsg(connection)
            );
        }

        sqlite3_bind_int(statement, 1, submissionId);
        result = sqlite3_step(statement);

        if (result != SQLITE_ROW)
        {
            string errorMessage = sqlite3_errmsg(connection);
            sqlite3_finalize(statement);

            if (result == SQLITE_DONE)
            {
                rollbackTransaction(connection);
                return false;
            }

            throw runtime_error(
                "Failed to retrieve submission for moderation: " +
                errorMessage
            );
        }

        bool hasMark =
            sqlite3_column_type(statement, 0) != SQLITE_NULL;

        double currentMark = hasMark
            ? sqlite3_column_double(statement, 0)
            : 0.0;

        string status = readTextColumn(statement, 1);
        sqlite3_finalize(statement);

        if (!hasMark ||
            !isfinite(currentMark) ||
            status != "Graded")
        {
            rollbackTransaction(connection);
            return false;
        }

        updateStatusAndAudit(
            connection,
            submissionId,
            hodId,
            "Graded",
            "Moderated",
            reason,
            currentMark
        );

        executeSql(connection, "COMMIT;");
        return true;
    }
    catch (...)
    {
        rollbackTransaction(connection);
        throw;
    }
}

// Releases a moderated submission after verifying HOD authorization.
bool SubmissionRepository::releaseSubmission(
    int submissionId,
    int hodId,
    const string& reason)
{
    if (submissionId <= 0 ||
        hodId <= 0 ||
        !hasValidReason(reason))
    {
        return false;
    }

    sqlite3* connection = database.getConnection();

    try
    {
        executeSql(connection, "BEGIN IMMEDIATE;");

        if (!isAuthorizedForSubmission(
                connection,
                submissionId,
                hodId,
                "HeadOfDepartment"))
        {
            rollbackTransaction(connection);
            return false;
        }

        const char* selectSql =
            "SELECT mark, status "
            "FROM submissions "
            "WHERE submission_id = ?;";

        sqlite3_stmt* statement = nullptr;

        int result = sqlite3_prepare_v2(
            connection,
            selectSql,
            -1,
            &statement,
            nullptr
        );

        if (result != SQLITE_OK)
        {
            throw runtime_error(
                string("Failed to prepare release query: ") +
                sqlite3_errmsg(connection)
            );
        }

        sqlite3_bind_int(statement, 1, submissionId);
        result = sqlite3_step(statement);

        if (result != SQLITE_ROW)
        {
            string errorMessage = sqlite3_errmsg(connection);
            sqlite3_finalize(statement);

            if (result == SQLITE_DONE)
            {
                rollbackTransaction(connection);
                return false;
            }

            throw runtime_error(
                "Failed to retrieve submission for release: " +
                errorMessage
            );
        }

        bool hasMark =
            sqlite3_column_type(statement, 0) != SQLITE_NULL;

        double currentMark = hasMark
            ? sqlite3_column_double(statement, 0)
            : 0.0;

        string status = readTextColumn(statement, 1);
        sqlite3_finalize(statement);

        if (!hasMark ||
            !isfinite(currentMark) ||
            status != "Moderated")
        {
            rollbackTransaction(connection);
            return false;
        }

        updateStatusAndAudit(
            connection,
            submissionId,
            hodId,
            "Moderated",
            "Released",
            reason,
            currentMark
        );

        executeSql(connection, "COMMIT;");
        return true;
    }
    catch (...)
    {
        rollbackTransaction(connection);
        throw;
    }
}
