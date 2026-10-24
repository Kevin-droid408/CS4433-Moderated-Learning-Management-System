
#include "models/Assignment.h"

Assignment::Assignment(int assignmentId,
                       int courseId,
                       const std::string& title,
                       const std::string& description,
                       const std::string& dueDate,
                       double maxMark)
    : assignmentId(assignmentId),
      courseId(courseId),
      title(title),
      description(description),
      dueDate(dueDate),
      maxMark(maxMark) {
}

int Assignment::getAssignmentId() const {
    return assignmentId;
}

int Assignment::getCourseId() const {
    return courseId;
}

const std::string& Assignment::getTitle() const {
    return title;
}

const std::string& Assignment::getDescription() const {
    return description;
}

const std::string& Assignment::getDueDate() const {
    return dueDate;
}

double Assignment::getMaxMark() const {
    return maxMark;
}
