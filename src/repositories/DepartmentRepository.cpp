
#include "repositories/DepartmentRepository.h"

#include <sqlite3.h>
#include <stdexcept>

DepartmentRepository::DepartmentRepository(Database& database)
    : database(database) {
}

bool DepartmentRepository::addDepartment(
    const std::string& departmentName) {

    const char* sql =
        "INSERT INTO departments (department_name) VALUES (?);";

    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        database.getConnection(),
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database.getConnection()));
    }

    sqlite3_bind_text(
        statement,
        1,
        departmentName.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    result = sqlite3_step(statement);

    sqlite3_finalize(statement);

    if (result == SQLITE_DONE) {
        return true;
    }

    if (result == SQLITE_CONSTRAINT) {
        return false;
    }

    throw std::runtime_error(sqlite3_errmsg(database.getConnection()));
}

std::vector<Department> DepartmentRepository::getAllDepartments() {
    std::vector<Department> departments;

    const char* sql =
        "SELECT department_id, department_name "
        "FROM departments ORDER BY department_name;";

    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        database.getConnection(),
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database.getConnection()));
    }

    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        Department department;

        department.departmentId = sqlite3_column_int(statement, 0);

        const unsigned char* name =
            sqlite3_column_text(statement, 1);

        department.departmentName =
            name ? reinterpret_cast<const char*>(name) : "";

        departments.push_back(department);
    }

    sqlite3_finalize(statement);

    if (result != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(database.getConnection()));
    }

    return departments;
}
