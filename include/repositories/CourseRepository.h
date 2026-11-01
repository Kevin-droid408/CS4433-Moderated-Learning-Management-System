
#ifndef COURSE_REPOSITORY_H
#define COURSE_REPOSITORY_H

#include "Database.h"
#include "models/Course.h"

#include <vector>

class CourseRepository {
private:
    Database& database;

public:
    explicit CourseRepository(Database& database);

    bool addCourse(const Course& course);
    std::vector<Course> getAllCourses();

    // Assign a lecturer to a course.
    // The lecturer and course must belong to the same department.
    bool assignLecturerToCourse(int lecturerId, int courseId);

    // Retrieve lecturer IDs assigned to a course.
    std::vector<int> getLecturersForCourse(int courseId);
};

#endif
