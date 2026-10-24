
#ifndef SUBMISSION_H
#define SUBMISSION_H

#include <optional>
#include <string>
using namespace std;

enum class SubmissionStatus {
    Submitted,
    Graded,
    Moderated,
    Released
};

class Submission {
    private:
        int submissionId;
        int studentId;
        int assignmentId;
        string submittedAt;
        string filePath;
        optional<double> mark;
        string feedback;
        SubmissionStatus status;

    public:
        Submission(int submissionId,
                int studentId,
                int assignmentId,
                const string& submittedAt,
                const string& filePath);

        int getSubmissionId() const;
        int getStudentId() const;
        int getAssignmentId() const;
        const string& getSubmittedAt() const;
        const string& getFilePath() const;
        const optional<double>& getMark() const;
        const string& getFeedback() const;
        SubmissionStatus getStatus() const;

        void setMark(double mark);
        void setFeedback(const string& feedback);
        void setStatus(SubmissionStatus status);
};

#endif
