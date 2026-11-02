#include <gtest/gtest.h>

#include "repositories/Database.h"
#include "repositories/AssignmentRepository.h"
#include "repositories/SubmissionRepository.h"
#include "models/Assignment.h"
#include "models/Submission.h"

#include <memory>
#include <string>

class SubmissionWorkflowIntegrationTest : public ::testing::Test {
protected:
    std::unique_ptr<Database> database;
    std::unique_ptr<AssignmentRepository> assignmentRepository;
    std::unique_ptr<SubmissionRepository> submissionRepository;

    int studentId = 1;
    int lecturerId = 2;
    int hodId = 3;
    int courseId = 1;
    int assignmentId = 0;
    int submissionId = 0;

    void SetUp() override {
        database = std::make_unique<Database>(":memory:");

        database->execute("PRAGMA foreign_keys = ON;");

        createTables();
        insertTestData();

        assignmentRepository =
            std::make_unique<AssignmentRepository>(*database);

        submissionRepository =
            std::make_unique<SubmissionRepository>(*database);

        createAssignmentAndSubmission();
    }

    void createTables() {
        database->execute(R"(
            CREATE TABLE departments (
                department_id INTEGER PRIMARY KEY AUTOINCREMENT,
                department_name TEXT NOT NULL UNIQUE
            );

            CREATE TABLE users (
                user_id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT NOT NULL UNIQUE,
                full_name TEXT NOT NULL,
                email TEXT NOT NULL UNIQUE,
                password_hash TEXT NOT NULL,
                role TEXT NOT NULL,
                department_id INTEGER,
                FOREIGN KEY (department_id)
                    REFERENCES departments(department_id)
            );

            CREATE TABLE courses (
                course_id INTEGER PRIMARY KEY AUTOINCREMENT,
                course_code TEXT NOT NULL UNIQUE,
                course_name TEXT NOT NULL,
                department_id INTEGER NOT NULL,
                FOREIGN KEY (department_id)
                    REFERENCES departments(department_id)
            );

            CREATE TABLE course_lecturers (
                lecturer_id INTEGER NOT NULL,
                course_id INTEGER NOT NULL,
                PRIMARY KEY (lecturer_id, course_id),
                FOREIGN KEY (lecturer_id)
                    REFERENCES users(user_id),
                FOREIGN KEY (course_id)
                    REFERENCES courses(course_id)
            );

            CREATE TABLE enrollments (
                student_id INTEGER NOT NULL,
                course_id INTEGER NOT NULL,
                PRIMARY KEY (student_id, course_id),
                FOREIGN KEY (student_id)
                    REFERENCES users(user_id),
                FOREIGN KEY (course_id)
                    REFERENCES courses(course_id)
            );

            CREATE TABLE assignments (
                assignment_id INTEGER PRIMARY KEY AUTOINCREMENT,
                course_id INTEGER NOT NULL,
                title TEXT NOT NULL,
                description TEXT,
                due_date TEXT NOT NULL,
                max_mark REAL NOT NULL CHECK(max_mark > 0),
                FOREIGN KEY (course_id)
                    REFERENCES courses(course_id)
            );

            CREATE TABLE extensions (
                extension_id INTEGER PRIMARY KEY AUTOINCREMENT,
                assignment_id INTEGER NOT NULL,
                student_id INTEGER NOT NULL,
                new_due_date TEXT NOT NULL,
                approved_by INTEGER NOT NULL,
                reason TEXT,
                UNIQUE(assignment_id, student_id),
                FOREIGN KEY (assignment_id)
                    REFERENCES assignments(assignment_id),
                FOREIGN KEY (student_id)
                    REFERENCES users(user_id),
                FOREIGN KEY (approved_by)
                    REFERENCES users(user_id)
            );

            CREATE TABLE submissions (
                submission_id INTEGER PRIMARY KEY AUTOINCREMENT,
                student_id INTEGER NOT NULL,
                assignment_id INTEGER NOT NULL,
                submitted_at TEXT NOT NULL,
                file_path TEXT NOT NULL,
                mark REAL,
                feedback TEXT DEFAULT '',
                status TEXT NOT NULL,
                FOREIGN KEY (student_id)
                    REFERENCES users(user_id),
                FOREIGN KEY (assignment_id)
                    REFERENCES assignments(assignment_id)
            );

            CREATE TABLE audit_logs (
                audit_id INTEGER PRIMARY KEY AUTOINCREMENT,
                submission_id INTEGER NOT NULL,
                changed_by INTEGER NOT NULL,
                changed_at TEXT NOT NULL,
                old_mark REAL,
                new_mark REAL,
                reason TEXT,
                FOREIGN KEY (submission_id)
                    REFERENCES submissions(submission_id),
                FOREIGN KEY (changed_by)
                    REFERENCES users(user_id)
            );
        )");
    }

    void insertTestData() {
        database->execute(R"(
            INSERT INTO departments (department_id, department_name)
            VALUES (1, 'Computer Science');

            INSERT INTO users
                (user_id, username, full_name, email,
                 password_hash, role, department_id)
            VALUES
                (1, 'student1', 'Test Student',
                 'student@test.com', 'test_hash',
                 'Student', 1),

                (2, 'lecturer1', 'Test Lecturer',
                 'lecturer@test.com', 'test_hash',
                 'Lecturer', 1),

                (3, 'hod1', 'Test HOD',
                 'hod@test.com', 'test_hash',
                 'HeadOfDepartment', 1);

            INSERT INTO courses
                (course_id, course_code, course_name, department_id)
            VALUES
                (1, 'CS101', 'Introduction to Computer Science', 1);

            INSERT INTO enrollments (student_id, course_id)
            VALUES (1, 1);

            INSERT INTO course_lecturers (lecturer_id, course_id)
            VALUES (2, 1);
        )");
    }

    void createAssignmentAndSubmission() {
        Assignment assignment(
            0,
            courseId,
            "Integration Testing",
            "Test assignment",
            "2026-10-01",
            100.0
        );

        ASSERT_TRUE(assignmentRepository->addAssignment(assignment));

        auto assignments =
            assignmentRepository->getAssignmentsByCourse(courseId);

        ASSERT_EQ(assignments.size(), 1);

        assignmentId = assignments[0].getAssignmentId();

        Submission submission(
            0,
            studentId,
            assignmentId,
            "2026-10-03 12:00:00",
            "submissions/test_assignment.pdf"
        );

        ASSERT_TRUE(submissionRepository->addSubmission(submission));

        auto submissions =
            submissionRepository->getSubmissionsByStudent(studentId);

        ASSERT_EQ(submissions.size(), 1);

        submissionId = submissions[0].getSubmissionId();
    }
};

TEST_F(
    SubmissionWorkflowIntegrationTest,
    StudentCanSubmitAssignment
) {
    auto submissions =
        submissionRepository->getSubmissionsByStudent(studentId);

    ASSERT_EQ(submissions.size(), 1);

    EXPECT_EQ(
        submissions[0].getStudentId(),
        studentId
    );

    EXPECT_EQ(
        submissions[0].getAssignmentId(),
        assignmentId
    );

    EXPECT_EQ(
        submissions[0].getStatus(),
        SubmissionStatus::Submitted
    );

    EXPECT_FALSE(submissions[0].getMark().has_value());
}

TEST_F(
    SubmissionWorkflowIntegrationTest,
    StudentCannotSeeMarkBeforeRelease
) {
    ASSERT_TRUE(
        submissionRepository->gradeSubmission(
            submissionId,
            80.0,
            "Good work",
            lecturerId,
            "Initial grading"
        )
    );

    auto submissions =
        submissionRepository->getSubmissionsByStudent(studentId);

    ASSERT_EQ(submissions.size(), 1);

    EXPECT_FALSE(submissions[0].getMark().has_value());
    EXPECT_TRUE(submissions[0].getFeedback().empty());
}

TEST_F(
    SubmissionWorkflowIntegrationTest,
    LecturerCanGradeAndLatePenaltyIsApplied
) {
    ASSERT_TRUE(
        submissionRepository->gradeSubmission(
            submissionId,
            80.0,
            "Good work",
            lecturerId,
            "Initial grading"
        )
    );

    GradePreview preview;

    ASSERT_TRUE(
        submissionRepository->previewGrade(
            submissionId,
            80.0,
            preview
        )
    );

    EXPECT_EQ(preview.daysLate, 2);
    EXPECT_DOUBLE_EQ(preview.awardedMark, 80.0);
    EXPECT_DOUBLE_EQ(preview.maximumMark, 100.0);
    EXPECT_DOUBLE_EQ(preview.finalMark, 60.0);
}

TEST_F(
    SubmissionWorkflowIntegrationTest,
    HeadOfDepartmentCanModerateSubmission
) {
    ASSERT_TRUE(
        submissionRepository->gradeSubmission(
            submissionId,
            80.0,
            "Good work",
            lecturerId,
            "Initial grading"
        )
    );

    EXPECT_TRUE(
        submissionRepository->moderateSubmission(
            submissionId,
            hodId,
            "Moderation completed"
        )
    );

    auto submissions =
        submissionRepository->getAllSubmissions();

    ASSERT_EQ(submissions.size(), 1);

    EXPECT_EQ(
        submissions[0].getStatus(),
        SubmissionStatus::Moderated
    );
}

TEST_F(
    SubmissionWorkflowIntegrationTest,
    HeadOfDepartmentCanReleaseSubmission
) {
    ASSERT_TRUE(
        submissionRepository->gradeSubmission(
            submissionId,
            80.0,
            "Good work",
            lecturerId,
            "Initial grading"
        )
    );

    ASSERT_TRUE(
        submissionRepository->moderateSubmission(
            submissionId,
            hodId,
            "Moderation completed"
        )
    );

    EXPECT_TRUE(
        submissionRepository->releaseSubmission(
            submissionId,
            hodId,
            "Approved for release"
        )
    );

    auto submissions =
        submissionRepository->getSubmissionsByStudent(studentId);

    ASSERT_EQ(submissions.size(), 1);

    EXPECT_EQ(
        submissions[0].getStatus(),
        SubmissionStatus::Released
    );

    ASSERT_TRUE(submissions[0].getMark().has_value());

    EXPECT_DOUBLE_EQ(
        submissions[0].getMark().value(),
        60.0
    );

    EXPECT_EQ(
        submissions[0].getFeedback(),
        "Good work"
    );
}