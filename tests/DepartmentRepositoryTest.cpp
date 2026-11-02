
#include <gtest/gtest.h>

#include "repositories/Database.h"
#include "repositories/DepartmentRepository.h"

#include <string>
#include <vector>

class DepartmentRepositoryTest : public ::testing::Test {
protected:
    Database database{":memory:"};
    DepartmentRepository departmentRepository{database};

    void SetUp() override {
        database.execute(
            "CREATE TABLE departments ("
            "department_id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "department_name TEXT NOT NULL UNIQUE"
            ");"
        );
    }
};

// --------------------------------------------------
// ADD DEPARTMENT TESTS
// --------------------------------------------------

TEST_F(DepartmentRepositoryTest, AddDepartmentSuccessfully) {
    EXPECT_TRUE(
        departmentRepository.addDepartment("Computer Science")
    );

    auto departments = departmentRepository.getAllDepartments();

    ASSERT_EQ(departments.size(), 1);
    EXPECT_EQ(departments[0].departmentName, "Computer Science");
}

TEST_F(DepartmentRepositoryTest, RejectDuplicateDepartmentName) {
    ASSERT_TRUE(
        departmentRepository.addDepartment("Computer Science")
    );

    EXPECT_FALSE(
        departmentRepository.addDepartment("Computer Science")
    );

    auto departments = departmentRepository.getAllDepartments();

    EXPECT_EQ(departments.size(), 1);
}

TEST_F(DepartmentRepositoryTest, AddMultipleDepartmentsSuccessfully) {
    EXPECT_TRUE(
        departmentRepository.addDepartment("Computer Science")
    );

    EXPECT_TRUE(
        departmentRepository.addDepartment("Information Technology")
    );

    EXPECT_TRUE(
        departmentRepository.addDepartment("Mathematics")
    );

    auto departments = departmentRepository.getAllDepartments();

    EXPECT_EQ(departments.size(), 3);
}

// --------------------------------------------------
// GET ALL DEPARTMENTS TESTS
// --------------------------------------------------

TEST_F(DepartmentRepositoryTest, GetAllDepartmentsReturnsEmptyList) {
    auto departments = departmentRepository.getAllDepartments();

    EXPECT_TRUE(departments.empty());
}

TEST_F(DepartmentRepositoryTest, GetAllDepartmentsReturnsAlphabeticalOrder) {
    ASSERT_TRUE(
        departmentRepository.addDepartment("Mathematics")
    );

    ASSERT_TRUE(
        departmentRepository.addDepartment("Computer Science")
    );

    ASSERT_TRUE(
        departmentRepository.addDepartment("Information Technology")
    );

    auto departments = departmentRepository.getAllDepartments();

    ASSERT_EQ(departments.size(), 3);

    EXPECT_EQ(departments[0].departmentName, "Computer Science");
    EXPECT_EQ(departments[1].departmentName, "Information Technology");
    EXPECT_EQ(departments[2].departmentName, "Mathematics");
}

TEST_F(DepartmentRepositoryTest, GetAllDepartmentsReturnsCorrectIds) {
    ASSERT_TRUE(
        departmentRepository.addDepartment("Computer Science")
    );

    ASSERT_TRUE(
        departmentRepository.addDepartment("Information Technology")
    );

    auto departments = departmentRepository.getAllDepartments();

    ASSERT_EQ(departments.size(), 2);

    EXPECT_GT(departments[0].departmentId, 0);
    EXPECT_GT(departments[1].departmentId, 0);

    EXPECT_NE(
        departments[0].departmentId,
        departments[1].departmentId
    );
}
