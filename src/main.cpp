
#include "repositories/Database.h"
#include "repositories/CourseRepository.h"

#include <iostream>
#include <exception>
#include <string>

int main() {
    try {
        // Step 1: Open the SQLite database.
        Database database("database/lms.db");

        database.initializeSchema("database/schema.sql");

        cout << "Database initialized successfully.\n";

        // Step 3: Insert a test department if it does not exist.
        database.execute(
            "INSERT OR IGNORE INTO departments "
            "(department_id, department_name) "
            "VALUES (1, 'Computer Science');"
        );

        // Step 4: Create the course repository.
        CourseRepository courseRepository(database);

        // Step 5: Prepare a test course.
        Course course(
            0,
            "CS101",
            "Introduction to Computer Science",
            1
        );

        // Step 6: Save the course.
        if (courseRepository.addCourse(course)) {
            std::cout << "Course saved successfully.\n";
        } else {
            std::cout
                << "Course was not saved. "
                << "It may already exist.\n";
        }

        // Step 7: Retrieve and display saved courses.
        std::cout << "\nCourses in the database:\n";

        const auto courses = courseRepository.getAllCourses();

        if (courses.empty()) {
            std::cout << "No courses found.\n";
        } else {
            for (const Course& savedCourse : courses) {
                std::cout
                    << "Course code: "
                    << savedCourse.getCourseCode()
                    << "\nCourse name: "
                    << savedCourse.getCourseName()
                    << "\nDepartment ID: "
                    << savedCourse.getDepartmentId()
                    << "\n-------------------------\n";
            }
        }

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "\nApplication error: "
                  << error.what() << '\n';

        return 1;
    }
}
