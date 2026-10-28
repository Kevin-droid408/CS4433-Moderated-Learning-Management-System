
#ifndef SUBMISSION_REPOSITORY_H
#define SUBMISSION_REPOSITORY_H

#include "Database.h"
#include "models/Submission.h"
#include "services/ThreeDayGraduatedLatePenaltyPolicy.h"

#include <string>
#include <vector>

// Contains the grading information shown to a lecturer
// before the grade is saved.
struct GradePreview {
    std::string submittedAt;
    std::string applicableDeadline;

    int daysLate = 0;

    double maximumMark = 0.0;
    double awardedMark = 0.0;
    double finalMark = 0.0;
};

class SubmissionRepository {
private:
    Database& database;

public:
    explicit SubmissionRepository(Database& database);

    // Creates a submission for an enrolled student.
    bool addSubmission(const Submission& submission);

    // Retrieves submissions belonging to the authenticated student.
    std::vector<Submission> getSubmissionsForAuthenticatedStudent(
        int studentId,
        int authenticatedUserId
    );

    // Retrieves submissions by student ID.
    std::vector<Submission> getSubmissionsByStudent(
        int studentId
    );

    // Retrieves all submissions.
    std::vector<Submission> getAllSubmissions();

    // Retrieves submissions belonging to a department.
    std::vector<Submission> getSubmissionsByDepartment(
        int departmentId
    );

    // Calculates the grading preview without saving a mark.
    // The supplied mark is the mark awarded before the late penalty.
    bool previewGrade(
        int submissionId,
        double awardedMark,
        GradePreview& preview
    );

    // Grades a submission, applies the late penalty,
    // and records the final mark in the audit log.
    // The supplied mark is the mark awarded before the penalty.
    bool gradeSubmission(
        int submissionId,
        double mark,
        const std::string& feedback,
        int lecturerId,
        const std::string& reason
    );

    // Allows the relevant Head of Department to moderate a grade.
    bool moderateSubmission(
        int submissionId,
        int hodId,
        const std::string& reason
    );

    // Releases a moderated grade to the student.
    bool releaseSubmission(
        int submissionId,
        int hodId,
        const std::string& reason
    );
};

#endif
