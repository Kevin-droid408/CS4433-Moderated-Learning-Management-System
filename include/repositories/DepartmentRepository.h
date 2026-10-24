
#ifndef DEPARTMENT_REPOSITORY_H
#define DEPARTMENT_REPOSITORY_H

#include "Database.h"

#include <string>
#include <vector>

using namespace std;

struct Department {
    int departmentId;
    string departmentName;
};

class DepartmentRepository {
    private:
        Database& database;

    public:
        explicit DepartmentRepository(Database& database);

        bool addDepartment(const string& departmentName);

        vector<Department> getAllDepartments();
};

#endif
