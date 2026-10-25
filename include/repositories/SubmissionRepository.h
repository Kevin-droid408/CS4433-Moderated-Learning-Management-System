
#ifndef SUBMISSION_REPOSITORY_H
#define SUBMISSION_REPOSITORY_H

#include "Database.h"
#include "models/Submission.h"

#include <string>
#include <vector>

class SubmissionRepository {
    private:
        Database& database;

    public:
        explicit SubmissionRepository(Database& database);

        bool addSubmission(const Submission& submission);

        std::vector<Submission> getSubmissionsByStudent(int studentId);

        std::vector<Submission> getAllSubmissions();

        // Grades a submission and records the lecturer's mark change.
        bool gradeSubmission(
            int submissionId,
            double mark,
            const std::string& feedback,
            int lecturerId,
            const std::string& reason
        );

        // Moderates a submission graded by a lecturer.
        bool moderateSubmission(
            int submissionId,
            int hodId,
            const std::string& reason
        );

        // Releases a moderated result to the student.
        bool releaseSubmission(
            int submissionId,
            int hodId,
            const std::string& reason
        );
};

#endif
