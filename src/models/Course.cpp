
#include "models/Course.h"

Course::Course(int courseId,
               const string& courseCode,
               const string& courseName,
               int departmentId)
    : courseId(courseId),
      courseCode(courseCode),
      courseName(courseName),
      departmentId(departmentId) {
}

int Course::getCourseId() const {
    return courseId;
}

const string& Course::getCourseCode() const {
    return courseCode;
}

const string& Course::getCourseName() const {
    return courseName;
}

int Course::getDepartmentId() const {
    return departmentId;
}
