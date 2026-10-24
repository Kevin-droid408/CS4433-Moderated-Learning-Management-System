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
        vector<Course> getAllCourses();
};

#endif