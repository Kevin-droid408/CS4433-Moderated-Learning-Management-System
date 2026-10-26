
#ifndef ENROLLMENTREPOSITORY_H
#define ENROLLMENTREPOSITORY_H

#include <sqlite3.h>
#include <vector>

class EnrollmentRepository {
private:
    sqlite3* db;

    bool isStudent(int studentId) const;
    bool courseExists(int courseId) const;

public:
    explicit EnrollmentRepository(sqlite3* database);

    bool enrollStudent(int studentId, int courseId);

    bool isStudentEnrolled(int studentId, int courseId) const;

    std::vector<int> getEnrolledCourses(int studentId) const;

    std::vector<int> getAvailableCourses(int studentId) const;
};

#endif
