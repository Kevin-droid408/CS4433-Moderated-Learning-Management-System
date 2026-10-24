
#include "models/Submission.h"

Submission::Submission(int submissionId,
                       int studentId,
                       int assignmentId,
                       const std::string& submittedAt,
                       const std::string& filePath)
    : submissionId(submissionId),
      studentId(studentId),
      assignmentId(assignmentId),
      submittedAt(submittedAt),
      filePath(filePath),
      mark(std::nullopt),
      feedback(""),
      status(SubmissionStatus::Submitted) {
}

int Submission::getSubmissionId() const {
    return submissionId;
}

int Submission::getStudentId() const {
    return studentId;
}

int Submission::getAssignmentId() const {
    return assignmentId;
}

const string& Submission::getSubmittedAt() const {
    return submittedAt;
}

const string& Submission::getFilePath() const {
    return filePath;
}

const optional<double>& Submission::getMark() const {
    return mark;
}

const string& Submission::getFeedback() const {
    return feedback;
}

SubmissionStatus Submission::getStatus() const {
    return status;
}

void Submission::setMark(double mark) {
    this->mark = mark;
}

void Submission::setFeedback(const std::string& feedback) {
    this->feedback = feedback;
}

void Submission::setStatus(SubmissionStatus status) {
    this->status = status;
}
