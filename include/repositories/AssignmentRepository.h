
#ifndef ASSIGNMENT_REPOSITORY_H
#define ASSIGNMENT_REPOSITORY_H

#include "Database.h"
#include "models/Assignment.h"

#include <vector>

class AssignmentRepository {
    private:
        Database& database;

    public:
        explicit AssignmentRepository(Database& database);

        bool addAssignment(const Assignment& assignment);

        std::vector<Assignment> getAllAssignments();
        std::vector<Assignment> getAssignmentsByCourse(int courseId);
};

#endif
