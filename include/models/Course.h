
#ifndef COURSE_H
#define COURSE_H

#include <string>
using namespace std;

class Course {
    private:
        int courseId;
        string courseCode;
        string courseName;
        int departmentId;

    public:
        Course(int courseId,
            const string& courseCode,
            const string& courseName,
            int departmentId);

        int getCourseId() const;
        const string& getCourseCode() const;
        const string& getCourseName() const;
        int getDepartmentId() const;
};

#endif
