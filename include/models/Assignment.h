
#ifndef ASSIGNMENT_H
#define ASSIGNMENT_H

#include <string>
using namespace std;

class Assignment {
    private:
        int assignmentId;
        int courseId;
        string title;
        string description;
        string dueDate;
        double maxMark;

    public:
        Assignment(int assignmentId,
                int courseId,
                const std::string& title,
                const std::string& description,
                const std::string& dueDate,
                double maxMark);

        int getAssignmentId() const;
        int getCourseId() const;
        const string& getTitle() const;
        const string& getDescription() const;
        const string& getDueDate() const;
        double getMaxMark() const;
};

#endif
