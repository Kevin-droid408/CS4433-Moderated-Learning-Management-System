#include "repositories/Database.h"
#include "repositories/UserRepository.h"
#include "repositories/CourseRepository.h"
#include "repositories/AssignmentRepository.h"
#include "repositories/SubmissionRepository.h"
#include "repositories/EnrollmentRepository.h"
#include "repositories/ExtensionRepository.h"
#include "repositories/DepartmentRepository.h"

#include "models/User.h"
#include "models/Course.h"
#include "models/Assignment.h"
#include "models/Submission.h"
#include "models/HeadOfDepartment.h"

#include "services/AuthenticationService.h"
#include "services/CreateAccount.h"
#include "services/PasswordService.h"

#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <exception>
#include <vector>
#include <fstream>
#include <ctime>
#include <cmath>
#include <stdexcept>
#include <optional>
#include <cctype>
#include <filesystem>
#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

using namespace std;

/* Reads and validates an integer menu choice. */
int readMenuChoice() {
    int choice;

    while (true) {
        cout << "Enter your choice: ";

        if (cin >> choice) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return choice;
        }

        if (cin.eof()) return -1;

        cout << "Invalid input. Please enter a number.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

/* Reads required text and rejects whitespace-only input. */
bool readRequiredText(const string& prompt, string& value) {
    while (true) {
        cout << prompt;

        if (!getline(cin, value)) return false;

        if (!value.empty() &&
            value.find_first_not_of(" \t\r\n") != string::npos) {
            return true;
        }

        cout << "This field cannot be empty. Please try again.\n";
    }
}

/* Reads a password without accepting an empty value.
   Note: standard console input does not hide the password. */
bool readPassword(string& password) {
    return readRequiredText("Enter password: ", password);
}

/* Reads a positive maximum mark. */
double readPositiveMark() {
    double mark;

    while (true) {
        cout << "Enter maximum mark: ";

        if (cin >> mark) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            if (isfinite(mark) && mark > 0) return mark;

            cout << "Maximum mark must be finite and greater than zero.\n";
        } else {
            if (cin.eof()) return -1;

            cout << "Invalid input. Enter a numeric mark.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }
}

/* Reads a mark within the assignment's permitted range. */
double readGradeMark(double maximumMark) {
    double mark;

    while (true) {
        cout << "Enter student's mark (0 to "
             << maximumMark << "): ";

        if (cin >> mark) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            if (isfinite(mark) && mark >= 0 && mark <= maximumMark) {
                return mark;
            }

            cout << "Invalid mark. Enter a value between 0 and "
                 << maximumMark << ".\n";
        } else {
            if (cin.eof()) return -1;

            cout << "Invalid input. Enter a numeric mark.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }
}

/* Creates a timestamp for a submission. */
string getCurrentTimestamp() {
    time_t now = time(nullptr);
    tm localTimeValue{};

    if (now == static_cast<time_t>(-1) ||
        localtime_s(&localTimeValue, &now) != 0) {
        throw runtime_error("Could not determine the current date and time.");
    }

    char buffer[20];

    if (strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S",
                 &localTimeValue) == 0) {
        throw runtime_error("Could not format the submission timestamp.");
    }

    return string(buffer);
}

/* Converts a submission status into readable text. */
string getSubmissionStatusText(SubmissionStatus status) {
    switch (status) {
        case SubmissionStatus::Submitted: return "Submitted";
        case SubmissionStatus::Graded: return "Graded";
        case SubmissionStatus::Moderated: return "Moderated";
        case SubmissionStatus::Released: return "Released";
    }

    return "Unknown";
}

/* Retrieves the authenticated user's department. */
bool getUserDepartment(
    const User& user,
    UserRepository& userRepository,
    int& departmentId) {

    optional<int> result =
        userRepository.getDepartmentId(user.getUserId());

    if (!result.has_value() || result.value() <= 0) {
        cout << "\nAccess denied: your account has no valid department assigned.\n";
        return false;
    }

    departmentId = result.value();
    return true;
}

/* Finds a course by ID in a list of courses. */
const Course* findCourse(const vector<Course>& courses, int courseId) {
    for (const Course& course : courses) {
        if (course.getCourseId() == courseId) return &course;
    }

    return nullptr;
}

/* Displays courses belonging to a department. */
void displayDepartmentCourses(
    CourseRepository& courseRepository,
    int departmentId) {

    vector<Course> courses = courseRepository.getAllCourses();
    bool found = false;

    cout << "\n========== MY DEPARTMENT COURSES ==========\n";

    for (const Course& course : courses) {
        if (course.getDepartmentId() != departmentId) continue;

        found = true;

        cout << "\nCourse ID: " << course.getCourseId()
             << "\nCourse code: " << course.getCourseCode()
             << "\nCourse name: " << course.getCourseName()
             << "\n-------------------------------------------\n";
    }

    if (!found) cout << "No courses are assigned to your department.\n";
}

/* Displays every course. */
void displayAvailableCourses(CourseRepository& courseRepository) {
    vector<Course> courses = courseRepository.getAllCourses();

    cout << "\n========== AVAILABLE COURSES ==========\n";

    if (courses.empty()) {
        cout << "No courses are currently available.\n";
        return;
    }

    for (const Course& course : courses) {
        cout << "\nCourse ID: " << course.getCourseId()
             << "\nCourse code: " << course.getCourseCode()
             << "\nCourse name: " << course.getCourseName()
             << "\n---------------------------------------\n";
    }
}

/* Displays a student's enrolled courses. */
void displayEnrolledCourses(
    const User& user,
    EnrollmentRepository& enrollmentRepository,
    CourseRepository& courseRepository) {

    vector<int> courseIds =
        enrollmentRepository.getEnrolledCourses(user.getUserId());

    vector<Course> courses = courseRepository.getAllCourses();

    cout << "\n========== MY ENROLLED COURSES ==========\n";

    if (courseIds.empty()) {
        cout << "You are not enrolled in any courses yet.\n";
        return;
    }

    for (int courseId : courseIds) {
        const Course* course = findCourse(courses, courseId);

        if (course != nullptr) {
            cout << "\nCourse ID: " << course->getCourseId()
                 << "\nCourse code: " << course->getCourseCode()
                 << "\nCourse name: " << course->getCourseName()
                 << "\n-----------------------------------------\n";
        }
    }
}

/* Allows a student to enroll in an available course. */
void enrollInCourse(
    const User& user,
    EnrollmentRepository& enrollmentRepository,
    CourseRepository& courseRepository) {

    vector<int> availableIds =
        enrollmentRepository.getAvailableCourses(user.getUserId());

    vector<Course> courses = courseRepository.getAllCourses();

    cout << "\n========== ENROLL IN A COURSE ==========\n";

    bool found = false;

    for (int courseId : availableIds) {
        const Course* course = findCourse(courses, courseId);
        if (course == nullptr) continue;

        found = true;

        cout << "\nCourse ID: " << course->getCourseId()
             << "\nCourse code: " << course->getCourseCode()
             << "\nCourse name: " << course->getCourseName()
             << "\n----------------------------------------\n";
    }

    if (!found) {
        cout << "There are no courses available for enrollment.\n";
        return;
    }

    cout << "Enter the course ID to enroll in (0 to cancel).\n";
    int courseId = readMenuChoice();

    if (courseId <= 0) {
        cout << "Enrollment cancelled.\n";
        return;
    }

    bool available = false;

    for (int id : availableIds) {
        if (id == courseId) {
            available = true;
            break;
        }
    }

    if (!available) {
        cout << "Invalid course ID or you are already enrolled in that course.\n";
        return;
    }

    if (enrollmentRepository.enrollStudent(user.getUserId(), courseId)) {
        cout << "You have successfully enrolled in the course.\n";
    } else {
        cout << "Enrollment failed. Please check the course and try again.\n";
    }
}

/* Returns assignments belonging only to a student's enrolled courses. */
vector<Assignment> getStudentAssignments(
    const User& user,
    EnrollmentRepository& enrollmentRepository,
    AssignmentRepository& assignmentRepository) {

    vector<int> enrolledIds =
        enrollmentRepository.getEnrolledCourses(user.getUserId());

    vector<Assignment> allAssignments =
        assignmentRepository.getAllAssignments();

    vector<Assignment> result;

    for (const Assignment& assignment : allAssignments) {
        for (int courseId : enrolledIds) {
            if (assignment.getCourseId() == courseId) {
                result.push_back(assignment);
                break;
            }
        }
    }

    return result;
}

/* Displays assignments belonging to enrolled courses. */
void displayStudentAssignments(
    const User& user,
    EnrollmentRepository& enrollmentRepository,
    AssignmentRepository& assignmentRepository) {

    vector<Assignment> assignments =
        getStudentAssignments(user, enrollmentRepository,
                             assignmentRepository);

    cout << "\n========== ASSIGNMENTS FOR MY COURSES ==========\n";

    if (assignments.empty()) {
        cout << "No assignments are available for your enrolled courses.\n";
        return;
    }

    for (const Assignment& assignment : assignments) {
        cout << "\nAssignment ID: " << assignment.getAssignmentId()
             << "\nCourse ID: " << assignment.getCourseId()
             << "\nTitle: " << assignment.getTitle()
             << "\nDescription: " << assignment.getDescription()
             << "\nDue date: " << assignment.getDueDate()
             << "\nMaximum mark: " << assignment.getMaxMark()
             << "\n-----------------------------------------------\n";
    }
}

/* Displays all assignments for staff. */
void displayAssignments(AssignmentRepository& assignmentRepository) {
    vector<Assignment> assignments =
        assignmentRepository.getAllAssignments();

    cout << "\n=============== ASSIGNMENTS ===============\n";

    if (assignments.empty()) {
        cout << "No assignments have been created yet.\n";
        return;
    }

    for (const Assignment& assignment : assignments) {
        cout << "\nAssignment ID: " << assignment.getAssignmentId()
             << "\nCourse ID: " << assignment.getCourseId()
             << "\nTitle: " << assignment.getTitle()
             << "\nDescription: " << assignment.getDescription()
             << "\nDue date: " << assignment.getDueDate()
             << "\nMaximum mark: " << assignment.getMaxMark()
             << "\n-------------------------------------------\n";
    }
}

/* Creates an assignment for a course in the lecturer's department. */
void createAssignment(
    CourseRepository& courseRepository,
    AssignmentRepository& assignmentRepository,
    int departmentId) {

    vector<Course> allCourses = courseRepository.getAllCourses();
    vector<Course> departmentCourses;

    for (const Course& course : allCourses) {
        if (course.getDepartmentId() == departmentId) {
            departmentCourses.push_back(course);
        }
    }

    cout << "\n========== CREATE ASSIGNMENT ==========\n";

    if (departmentCourses.empty()) {
        cout << "Your department has no available courses.\n";
        return;
    }

    for (const Course& course : departmentCourses) {
        cout << "\nID: " << course.getCourseId()
             << " | " << course.getCourseCode()
             << " - " << course.getCourseName() << '\n';
    }

    cout << "\nEnter 0 to cancel.\n";
    int courseId = readMenuChoice();

    if (courseId <= 0) {
        cout << "Assignment creation cancelled.\n";
        return;
    }

    if (findCourse(departmentCourses, courseId) == nullptr) {
        cout << "Invalid course ID or course outside your department.\n";
        return;
    }

    string title, description, dueDate;

    if (!readRequiredText("Enter assignment title: ", title) ||
        !readRequiredText("Enter assignment description: ", description) ||
        !readRequiredText("Enter due date (YYYY-MM-DD): ", dueDate)) {
        cout << "\nInput ended. Assignment creation cancelled.\n";
        return;
    }

    double maxMark = readPositiveMark();

    if (maxMark <= 0) {
        cout << "Assignment creation cancelled.\n";
        return;
    }

    Assignment assignment(0, courseId, title, description, dueDate, maxMark);

    if (assignmentRepository.addAssignment(assignment)) {
        cout << "Assignment created successfully!\n";
    } else {
        cout << "Assignment could not be saved.\n";
    }
}

/* Grants an individual extension to an enrolled student. */
void grantIndividualExtension(
    const User& lecturer,
    ExtensionRepository& extensionRepository) {

    cout << "\n========== GRANT INDIVIDUAL EXTENSION ==========\n"
         << "Enter 0 at the assignment ID prompt to cancel.\n";

    int assignmentId = readMenuChoice();

    if (assignmentId <= 0) {
        cout << "Extension request cancelled.\n";
        return;
    }

    cout << "Enter student ID (0 to cancel): ";
    int studentId = readMenuChoice();

    if (studentId <= 0) {
        cout << "Extension request cancelled.\n";
        return;
    }

    string newDueDate;
    string reason;

    if (!readRequiredText(
            "Enter the new deadline (YYYY-MM-DD): ", newDueDate) ||
        !readRequiredText(
            "Enter the reason for granting this extension: ", reason)) {
        cout << "\nInput ended. Extension request cancelled.\n";
        return;
    }

    cout << "\nPlease confirm the extension:\n"
         << "Assignment ID: " << assignmentId
         << "\nStudent ID: " << studentId
         << "\nNew deadline: " << newDueDate
         << "\nReason: " << reason
         << "\n1. Confirm and save"
         << "\n0. Cancel\n";

    if (readMenuChoice() != 1) {
        cout << "Extension request cancelled.\n";
        return;
    }

    bool success = extensionRepository.grantIndividualExtension(
        assignmentId,
        studentId,
        newDueDate,
        lecturer.getUserId(),
        reason);

    if (success) {
        cout << "\nIndividual extension granted successfully.\n";
        cout << "The new deadline will be used when calculating the "
                "student's late penalty.\n";
    } else {
        cout << "\nExtension could not be granted.\n"
             << "Check that the assignment and student exist, the student "
                "is enrolled in the assignment's course, the assignment "
                "belongs to your department, and the deadline is valid.\n";
    }
}

/* Allows a student to submit only assignments for enrolled courses. */
void submitAssignment(
    const User& user,
    EnrollmentRepository& enrollmentRepository,
    AssignmentRepository& assignmentRepository,
    SubmissionRepository& submissionRepository) {

    vector<Assignment> assignments =
        getStudentAssignments(user, enrollmentRepository,
                             assignmentRepository);

    cout << "\n========== SUBMIT ASSIGNMENT ==========\n";

    if (assignments.empty()) {
        cout << "There are no assignments available for your enrolled courses.\n";
        return;
    }

    for (const Assignment& assignment : assignments) {
        cout << "\nAssignment ID: " << assignment.getAssignmentId()
             << "\nCourse ID: " << assignment.getCourseId()
             << "\nTitle: " << assignment.getTitle()
             << "\nDue date: " << assignment.getDueDate()
             << "\nMaximum mark: " << assignment.getMaxMark()
             << "\n---------------------------------------\n";
    }

    cout << "Enter assignment ID to submit (0 to cancel).\n";
    int assignmentId = readMenuChoice();

    if (assignmentId <= 0) {
        cout << "Submission cancelled.\n";
        return;
    }

    const Assignment* selected = nullptr;

    for (const Assignment& assignment : assignments) {
        if (assignment.getAssignmentId() == assignmentId) {
            selected = &assignment;
            break;
        }
    }

    if (selected == nullptr) {
        cout << "That assignment is not available for your enrolled courses.\n";
        return;
    }

    string filePath;

    if (!readRequiredText("Enter the path to your submission file: ",
                          filePath)) {
        cout << "\nInput ended. Submission cancelled.\n";
        return;
    }

    ifstream submissionFile(filePath, ios::binary);

    if (!submissionFile.is_open()) {
        cout << "The file could not be opened. Check the path and try again.\n";
        return;
    }

    submissionFile.close();

    Submission submission(
        0, user.getUserId(), assignmentId,
        getCurrentTimestamp(), filePath
    );

    if (submissionRepository.addSubmission(submission)) {
        cout << "Assignment submitted successfully!\nStatus: Submitted\n";
    } else {
        cout << "Submission failed. Check the assignment and database constraints.\n";
    }
}

/* Displays a student's own submissions. */
void displayMySubmissions(
    const User& user,
    SubmissionRepository& submissionRepository,
    bool releasedOnly = false) {

    vector<Submission> submissions =
        submissionRepository.getSubmissionsForAuthenticatedStudent(
            user.getUserId(), user.getUserId());

    cout << (releasedOnly
        ? "\n========== MY RELEASED MARKS ==========\n"
        : "\n========== MY SUBMISSIONS ==========\n");

    bool found = false;

    for (const Submission& submission : submissions) {
        if (releasedOnly &&
            submission.getStatus() != SubmissionStatus::Released) {
            continue;
        }

        found = true;

        cout << "\nSubmission ID: " << submission.getSubmissionId()
             << "\nAssignment ID: " << submission.getAssignmentId()
             << "\nSubmitted at: " << submission.getSubmittedAt()
             << "\nStatus: " << getSubmissionStatusText(submission.getStatus());

        if (submission.getStatus() == SubmissionStatus::Released) {
            if (submission.getMark().has_value()) {
                cout << "\nMark: " << submission.getMark().value();
            } else {
                cout << "\nMark: Not available";
            }

            cout << "\nFeedback: "
                 << (submission.getFeedback().empty()
                     ? "No feedback provided" : submission.getFeedback());
        } else if (!releasedOnly) {
            cout << "\nMark: Not yet released"
                 << "\nFeedback: Not yet released";
        }

        cout << "\n------------------------------------\n";
    }

    if (!found) {
        cout << (releasedOnly
            ? "No marks have been released yet.\n"
            : "No matching submissions were found.\n");
    }
}

/* Displays department submissions. */
void displayDepartmentSubmissions(
    SubmissionRepository& submissionRepository,
    int departmentId) {

    vector<Submission> submissions =
        submissionRepository.getSubmissionsByDepartment(departmentId);

    cout << "\n========== DEPARTMENT SUBMISSIONS ==========\n";

    if (submissions.empty()) {
        cout << "No submissions were found for your department.\n";
        return;
    }

    for (const Submission& submission : submissions) {
        cout << "\nSubmission ID: " << submission.getSubmissionId()
             << "\nStudent ID: " << submission.getStudentId()
             << "\nAssignment ID: " << submission.getAssignmentId()
             << "\nSubmitted at: " << submission.getSubmittedAt()
             << "\nFile path: " << submission.getFilePath()
             << "\nStatus: " << getSubmissionStatusText(submission.getStatus());

        if (submission.getMark().has_value()) {
            cout << "\nMark: " << submission.getMark().value();
        } else {
            cout << "\nMark: Not yet assigned";
        }

        if (!submission.getFeedback().empty()) {
            cout << "\nFeedback: " << submission.getFeedback();
        }

        cout << "\n--------------------------------------------\n";
    }
}

/* Grades a submission from the lecturer's department. */
void gradeSubmission(
    const User& lecturer,
    AssignmentRepository& assignmentRepository,
    SubmissionRepository& submissionRepository,
    int departmentId) {

    vector<Submission> submissions =
        submissionRepository.getSubmissionsByDepartment(departmentId);

    cout << "\n========== GRADE SUBMISSION ==========\n";
    bool available = false;

    for (const Submission& submission : submissions) {
        if (submission.getStatus() == SubmissionStatus::Submitted ||
            submission.getStatus() == SubmissionStatus::Graded) {
            available = true;

            cout << "\nSubmission ID: " << submission.getSubmissionId()
                 << "\nStudent ID: " << submission.getStudentId()
                 << "\nAssignment ID: " << submission.getAssignmentId()
                 << "\nStatus: " << getSubmissionStatusText(submission.getStatus())
                 << "\n--------------------------------------\n";
        }
    }

    if (!available) {
        cout << "No submissions are available for grading.\n";
        return;
    }

    cout << "Enter submission ID to grade (0 to cancel).\n";
    int submissionId = readMenuChoice();

    if (submissionId <= 0) {
        cout << "Grading cancelled.\n";
        return;
    }

    const Submission* selectedSubmission = nullptr;

    for (const Submission& submission : submissions) {
        if (submission.getSubmissionId() == submissionId) {
            selectedSubmission = &submission;
            break;
        }
    }

    if (selectedSubmission == nullptr ||
        (selectedSubmission->getStatus() != SubmissionStatus::Submitted &&
         selectedSubmission->getStatus() != SubmissionStatus::Graded)) {
        cout << "Submission not found or not available for grading.\n";
        return;
    }

    vector<Assignment> assignments =
        assignmentRepository.getAllAssignments();

    const Assignment* selectedAssignment = nullptr;

    for (const Assignment& assignment : assignments) {
        if (assignment.getAssignmentId() ==
            selectedSubmission->getAssignmentId()) {
            selectedAssignment = &assignment;
            break;
        }
    }

    if (selectedAssignment == nullptr) {
        cout << "The associated assignment could not be found.\n";
        return;
    }

    double mark = readGradeMark(selectedAssignment->getMaxMark());

    if (mark < 0) {
        cout << "Grading cancelled.\n";
        return;
    }

    string feedback, reason;

    if (!readRequiredText("Enter feedback for the student: ", feedback) ||
        !readRequiredText("Enter the reason for this mark change: ", reason)) {
        cout << "\nInput ended. Grading cancelled.\n";
        return;
    }

    cout << "\nConfirm grading:\n"
         << "Submission ID: " << submissionId
         << "\nMark: " << mark << " / " << selectedAssignment->getMaxMark()
         << "\n1. Save mark\n0. Cancel\n";

    if (readMenuChoice() != 1) {
        cout << "Grading cancelled.\n";
        return;
    }

    if (submissionRepository.gradeSubmission(
            submissionId, mark, feedback, lecturer.getUserId(), reason)) {
        cout << "Mark saved. It is awaiting HOD moderation.\n";
    } else {
        cout << "The mark could not be saved. Check authorization and status.\n";
    }
}

/* Displays graded submissions awaiting HOD moderation. */
void displaySubmissionsAwaitingModeration(
    SubmissionRepository& submissionRepository,
    int departmentId) {

    vector<Submission> submissions =
        submissionRepository.getSubmissionsByDepartment(departmentId);

    cout << "\n========== SUBMISSIONS AWAITING MODERATION ==========\n";
    bool found = false;

    for (const Submission& submission : submissions) {
        if (submission.getStatus() != SubmissionStatus::Graded) continue;

        found = true;

        cout << "\nSubmission ID: " << submission.getSubmissionId()
             << "\nStudent ID: " << submission.getStudentId()
             << "\nAssignment ID: " << submission.getAssignmentId()
             << "\nMark: ";

        if (submission.getMark().has_value()) {
            cout << submission.getMark().value();
        } else {
            cout << "Not assigned";
        }

        cout << "\nFeedback: " << submission.getFeedback()
             << "\nStatus: Graded\n-----------------------------------------------\n";
    }

    if (!found) cout << "No submissions are awaiting moderation.\n";
}

/* Allows an HOD to moderate or release submissions. */
void processHODSubmission(
    const User& hod,
    SubmissionRepository& submissionRepository,
    int departmentId,
    bool releaseResult) {

    if (hod.getRole() != "HeadOfDepartment") {
        cout << "Access denied. Only the HOD can perform this action.\n";
        return;
    }

    vector<Submission> submissions =
        submissionRepository.getSubmissionsByDepartment(departmentId);

    SubmissionStatus requiredStatus = releaseResult
        ? SubmissionStatus::Moderated : SubmissionStatus::Graded;

    cout << (releaseResult
        ? "\n========== RELEASE MODERATED RESULT ==========\n"
        : "\n========== MODERATE GRADED SUBMISSION ==========\n");

    bool found = false;

    for (const Submission& submission : submissions) {
        if (submission.getStatus() != requiredStatus) continue;

        found = true;

        cout << "\nSubmission ID: " << submission.getSubmissionId()
             << "\nStudent ID: " << submission.getStudentId()
             << "\nAssignment ID: " << submission.getAssignmentId()
             << "\nStatus: " << getSubmissionStatusText(submission.getStatus())
             << "\nMark: ";

        if (submission.getMark().has_value()) {
            cout << submission.getMark().value();
        } else {
            cout << "Not assigned";
        }

        cout << "\nFeedback: " << submission.getFeedback()
             << "\n-----------------------------------------------\n";
    }

    if (!found) {
        cout << "No submissions are available for this operation.\n";
        return;
    }

    cout << "Enter submission ID (0 to cancel).\n";
    int submissionId = readMenuChoice();

    if (submissionId <= 0) {
        cout << "Operation cancelled.\n";
        return;
    }

    const Submission* selectedSubmission = nullptr;

    for (const Submission& submission : submissions) {
        if (submission.getSubmissionId() == submissionId) {
            selectedSubmission = &submission;
            break;
        }
    }

    if (selectedSubmission == nullptr ||
        selectedSubmission->getStatus() != requiredStatus) {
        cout << "Invalid submission ID or status.\n";
        return;
    }

    string reason;

    if (!readRequiredText(
            releaseResult ? "Reason for releasing this result: "
                          : "Enter moderation reason: ",
            reason)) {
        cout << "\nInput ended. Operation cancelled.\n";
        return;
    }

    cout << "1. Confirm\n0. Cancel\n";

    if (readMenuChoice() != 1) {
        cout << "Operation cancelled.\n";
        return;
    }

    bool success = releaseResult
        ? submissionRepository.releaseSubmission(
              submissionId, hod.getUserId(), reason)
        : submissionRepository.moderateSubmission(
              submissionId, hod.getUserId(), reason);

    cout << (success
        ? "\nOperation completed successfully.\n"
        : "\nOperation failed. Check authorization and submission status.\n");
}

void changeHODMark(
    const User& hod,
    AssignmentRepository& assignmentRepository,
    SubmissionRepository& submissionRepository,
    int departmentId
) {
    if (hod.getRole() != "HeadOfDepartment") {
        cout << "Access denied.\n";
        return;
    }

    vector<Submission> submissions =
        submissionRepository.getSubmissionsByDepartment(departmentId);

    cout << "\n========== CHANGE A MARK ==========\n";

    bool found = false;

    for (const Submission& submission : submissions) {
        if ((submission.getStatus() == SubmissionStatus::Graded ||
             submission.getStatus() == SubmissionStatus::Moderated) &&
            submission.getMark().has_value()) {

            found = true;

            cout << "\nSubmission ID: "
                 << submission.getSubmissionId()
                 << "\nStudent ID: " << submission.getStudentId()
                 << "\nAssignment ID: " << submission.getAssignmentId()
                 << "\nCurrent mark: " << submission.getMark().value()
                 << "\nStatus: "
                 << getSubmissionStatusText(submission.getStatus())
                 << "\n----------------------------------\n";
        }
    }

    if (!found) {
        cout << "No eligible marks are available to change.\n";
        return;
    }

    cout << "Enter submission ID (0 to cancel): ";
    int submissionId = readMenuChoice();

    if (submissionId <= 0) {
        cout << "Mark change cancelled.\n";
        return;
    }

    const Submission* selected = nullptr;

    for (const Submission& submission : submissions) {
        if (submission.getSubmissionId() == submissionId) {
            selected = &submission;
            break;
        }
    }

    if (selected == nullptr ||
        !selected->getMark().has_value() ||
        (selected->getStatus() != SubmissionStatus::Graded &&
         selected->getStatus() != SubmissionStatus::Moderated)) {
        cout << "Invalid submission or status.\n";
        return;
    }

    vector<Assignment> assignments =
        assignmentRepository.getAllAssignments();

    const Assignment* selectedAssignment = nullptr;

    for (const Assignment& assignment : assignments) {
        if (assignment.getAssignmentId() ==
            selected->getAssignmentId()) {
            selectedAssignment = &assignment;
            break;
        }
    }

    if (selectedAssignment == nullptr) {
        cout << "Assignment not found.\n";
        return;
    }

    double newMark = readGradeMark(
        selectedAssignment->getMaxMark()
    );

    if (newMark < 0) {
        cout << "Mark change cancelled.\n";
        return;
    }

    string reason;

    if (!readRequiredText(
            "Enter the reason for changing this mark: ", reason)) {
        cout << "Mark change cancelled.\n";
        return;
    }

    cout << "\nConfirm mark change:\n"
         << "Old mark: " << selected->getMark().value()
         << "\nNew mark: " << newMark
         << "\n1. Save change\n"
         << "0. Cancel\n";

    if (readMenuChoice() != 1) {
        cout << "Mark change cancelled.\n";
        return;
    }

    bool success = submissionRepository.changeSubmissionMark(
        submissionId,
        hod.getUserId(),
        newMark,
        reason
    );

    cout << (success
        ? "Mark changed successfully. The result is now moderated.\n"
        : "Mark change failed. Check authorization and submission status.\n");
}


/* Displays all departments and returns the selected department ID.
   Returns 0 when the selection is cancelled or invalid. */
int selectDepartment(DepartmentRepository& departmentRepository) {
    vector<Department> departments =
        departmentRepository.getAllDepartments();

    cout << "\n========== SELECT DEPARTMENT ==========\n";

    if (departments.empty()) {
        cout << "No departments are available. Add a department first.\n";
        return 0;
    }

    for (const Department& department : departments) {
        cout << department.departmentId << ". "
             << department.departmentName << '\n';
    }

    cout << "0. Cancel\n";

    int choice = readMenuChoice();

    if (choice <= 0) {
        cout << "Department selection cancelled.\n";
        return 0;
    }

    for (const Department& department : departments) {
        if (department.departmentId == choice) {
            return department.departmentId;
        }
    }

    cout << "Invalid department ID.\n";
    return 0;
}

/* Displays all departments. */
void manageDepartments(DepartmentRepository& departmentRepository) {
    while (true) {
        cout << "\n========== MANAGE DEPARTMENTS ==========\n"
             << "1. View departments\n"
             << "2. Add department\n"
             << "0. Back\n"
             << "========================================\n";

        int choice = readMenuChoice();

        if (choice <= 0) {
            return;
        }

        if (choice == 1) {
            vector<Department> departments =
                departmentRepository.getAllDepartments();

            cout << "\n========== DEPARTMENTS ==========\n";

            if (departments.empty()) {
                cout << "No departments found.\n";
            } else {
                for (const Department& department : departments) {
                    cout << "Department ID: "
                         << department.departmentId
                         << "\nName: "
                         << department.departmentName
                         << "\n------------------------------\n";
                }
            }
        } else if (choice == 2) {
            string name;

            if (!readRequiredText("Enter department name: ", name)) {
                cout << "Input ended. Returning to the previous menu.\n";
                return;
            }

            if (departmentRepository.addDepartment(name)) {
                cout << "Department added successfully.\n";
            } else {
                cout << "Department could not be added. "
                     << "Check whether it already exists.\n";
            }
        } else {
            cout << "Invalid choice.\n";
        }
    }
}

/* Displays courses and allows the administrator to add courses. */
void manageCourses(
    CourseRepository& courseRepository,
    DepartmentRepository& departmentRepository) {

    while (true) {
        cout << "\n========== MANAGE COURSES ==========\n"
             << "1. View courses\n"
             << "2. Add course\n"
             << "0. Back\n"
             << "====================================\n";

        int choice = readMenuChoice();

        if (choice <= 0) {
            return;
        }

        if (choice == 1) {
            vector<Course> courses = courseRepository.getAllCourses();
            vector<Department> departments =
                departmentRepository.getAllDepartments();

            cout << "\n========== ALL COURSES ==========\n";

            if (courses.empty()) {
                cout << "No courses found.\n";
            } else {
                for (const Course& course : courses) {
                    string departmentName = "Unknown";

                    for (const Department& department : departments) {
                        if (department.departmentId ==
                            course.getDepartmentId()) {
                            departmentName = department.departmentName;
                            break;
                        }
                    }

                    cout << "Course ID: " << course.getCourseId()
                         << "\nCode: " << course.getCourseCode()
                         << "\nName: " << course.getCourseName()
                         << "\nDepartment: " << departmentName
                         << "\n------------------------------\n";
                }
            }
        } else if (choice == 2) {
            int departmentId =
                selectDepartment(departmentRepository);

            if (departmentId <= 0) {
                continue;
            }

            string code;
            string name;

            if (!readRequiredText("Enter course code: ", code) ||
                !readRequiredText("Enter course name: ", name)) {
                cout << "Input ended. Course creation cancelled.\n";
                return;
            }

            Course course(0, code, name, departmentId);

            if (courseRepository.addCourse(course)) {
                cout << "Course added successfully.\n";
            } else {
                cout << "Course could not be added. "
                     << "Check the course code and department.\n";
            }
        } else {
            cout << "Invalid choice.\n";
        }
    }
}

/* Creates an HOD account and assigns it to a selected department. */
void createHODAccount(
    UserRepository& userRepository,
    DepartmentRepository& departmentRepository) {

    cout << "\n========== CREATE HOD ACCOUNT ==========\n"
         << "Enter 0 at the username prompt to cancel.\n";

    string username;

    if (!readRequiredText("Enter username: ", username)) {
        cout << "Input ended. Account creation cancelled.\n";
        return;
    }

    if (username == "0") {
        cout << "Account creation cancelled.\n";
        return;
    }

    if (username.length() < 3 || username.length() > 30) {
        cout << "Username must contain 3 to 30 characters.\n";
        return;
    }

    if (userRepository.usernameExists(username)) {
        cout << "That username is already taken.\n";
        return;
    }

    string fullName;
    string email;
    string password;
    string confirmation;

    if (!readRequiredText("Enter full name: ", fullName) ||
        !readRequiredText("Enter email: ", email) ||
        !readPassword(password)) {
        cout << "Input ended. Account creation cancelled.\n";
        return;
    }

    if (userRepository.emailExists(email)) {
        cout << "An account with that email already exists.\n";
        return;
    }

    if (password.length() < 8) {
        cout << "Use a password with at least 8 characters.\n";
        return;
    }

    if (!readRequiredText("Confirm password: ", confirmation)) {
        cout << "Input ended. Account creation cancelled.\n";
        return;
    }

    if (password != confirmation) {
        cout << "Passwords do not match.\n";
        return;
    }

    int departmentId = selectDepartment(departmentRepository);

    if (departmentId <= 0) {
        cout << "HOD account creation cancelled.\n";
        return;
    }

    string passwordHash = PasswordService::hashPassword(password);

    HeadOfDepartment hod(0, fullName, email, passwordHash);

    if (userRepository.addUser(
            hod, username, passwordHash, departmentId)) {
        cout << "\nHOD account created successfully.\n"
             << "Username: " << username
             << "\nDepartment ID: " << departmentId << '\n';
    } else {
        cout << "HOD account creation failed. "
             << "Check the supplied details and database constraints.\n";
    }
}

/* Administrator's user-management submenu. */
void manageUsers(
    UserRepository& userRepository,
    DepartmentRepository& departmentRepository) {

    while (true) {
        cout << "\n========== MANAGE USERS ==========\n"
             << "1. Create Head of Department account\n"
             << "0. Back\n"
             << "==================================\n";

        int choice = readMenuChoice();

        if (choice <= 0) {
            return;
        }

        if (choice == 1) {
            createHODAccount(userRepository, departmentRepository);
        } else {
            cout << "Invalid choice.\n";
        }
    }
}


/* Displays a role-specific menu. */
void showRoleMenu(const User& user) {
    cout << "\n========== " << user.getRole() << " MENU ==========\n";

    if (user.getRole() == "Student") {
        cout << "1. View available courses\n"
             << "2. Enroll in a course\n"
             << "3. View my enrolled courses\n"
             << "4. View assignments for my courses\n"
             << "5. Submit assignment\n"
             << "6. View my submission status\n"
             << "7. View released marks\n";
    } else if (user.getRole() == "Lecturer") {
        cout << "1. View my department courses\n"
             << "2. Create assignment\n"
             << "3. View department submissions\n"
             << "4. Grade submission\n"
             << "5. Grant individual extension\n";
    } 
     else if (user.getRole() == "HeadOfDepartment") {
        cout << "1. View marks awaiting moderation\n"
             << "2. Moderate marks\n"
             << "3. Release marks\n"
             << "4. Change a mark\n";
    }
    else if (user.getRole() == "Administrator") {
        cout << "1. Manage users\n"
             << "2. Manage departments\n"
             << "3. Manage courses\n";
    }

    cout << "0. Log out\n"
         << "========================================\n";
}

int getHighestRoleOption(const User& user) {
    if (user.getRole() == "Student") 
        return 7;
    if (user.getRole() == "Lecturer") 
        return 5;

    if (user.getRole() == "HeadOfDepartment") 
        return 4;
    if (user.getRole() == "Administrator") 
        return 4;

    return -1;
}

/* Creates an account using the CreateAccount service. */
void createAccountMenu(CreateAccount& accountService) {
    cout << "\n========== CREATE ACCOUNT ==========\n"
         << "1. Student\n"
         << "2. Lecturer\n"
         << "0. Cancel\n";

    int choice = readMenuChoice();

    if (choice == -1 || choice == 0) {
        cout << "Account creation cancelled.\n";
        return;
    }

    string role;

    if (choice == 1) {
        role = "Student";
    } else if (choice == 2) {
        role = "Lecturer";
    } else {
        cout << "Invalid account type.\n";
        return;
    }

    string username, fullName, email, password;

    if (!readRequiredText("Choose a username (3-30 characters): ", username) ||
        !readRequiredText("Enter your full name: ", fullName) ||
        !readRequiredText("Enter your email address: ", email) ||
        !readPassword(password)) {
        cout << "\nInput ended. Account creation cancelled.\n";
        return;
    }

    int departmentId = 0;

    if (role == "Lecturer") {
        cout << "\nSelect your department:\n"
             << "1. Computer Science\n"
             << "2. Information Technology\n"
             << "0. Cancel\n";

        int departmentChoice = readMenuChoice();

        if (departmentChoice == -1 || departmentChoice == 0) {
            cout << "Account creation cancelled.\n";
            return;
        }

        if (departmentChoice == 1) {
            departmentId = 1;
        } else if (departmentChoice == 2) {
            departmentId = 2;
        } else {
            cout << "Invalid department selection.\n";
            return;
        }
    }

    bool success = accountService.registerAccount(
        username,
        fullName,
        email,
        password,
        role,
        departmentId
    );

    if (success) {
        cout << "\nAccount created successfully!\n"
             << "You can now log in using your username and password.\n";
    } else {
        cout << "\nAccount creation failed.\n"
             << "Check your information, username and email uniqueness, "
                "and the selected department.\n";
    }
}

int main() {
    try {
        // Resolve database and schema paths from the project root.
        const filesystem::path projectRoot = PROJECT_ROOT;

        const filesystem::path databaseDirectory =
            projectRoot / "database";

        const filesystem::path databasePath =
            databaseDirectory / "lms.db";

        const filesystem::path schemaPath =
            databaseDirectory / "schema.sql";

        // Ensure the database directory exists.
        filesystem::create_directories(databaseDirectory);

        // Initialize the database and schema.
        Database database(databasePath.string());
        database.initializeSchema(schemaPath.string());

        // Initialize repositories.
        UserRepository userRepository(database);
        CourseRepository courseRepository(database);
        AssignmentRepository assignmentRepository(database);
        SubmissionRepository submissionRepository(database);
        ExtensionRepository extensionRepository(database);
        DepartmentRepository departmentRepository(database);

        EnrollmentRepository enrollmentRepository(
            database.getConnection());

        // Initialize services.
        AuthenticationService authenticationService(userRepository);
        CreateAccount accountService(userRepository);

        bool applicationRunning = true;

        while (applicationRunning) {
            cout << "\n========== MODERATED LMS ==========\n"
                 << "1. Login\n"
                 << "2. Create Account\n"
                 << "0. Exit\n"
                 << "===================================\n";

            int mainChoice = readMenuChoice();

            if (mainChoice == -1 || mainChoice == 0) {
                applicationRunning = false;
                cout << "\nExiting the Moderated LMS. Goodbye!\n";
            }
            else if (mainChoice == 2) {
                createAccountMenu(accountService);
            }
            else if (mainChoice == 1) {
                string username;
                string password;

                if (!readRequiredText(
                        "Enter your username: ", username) ||
                    !readPassword(password)) {
                    cout << "\nInput ended. Exiting the application.\n";
                    return 0;
                }

                unique_ptr<User> loggedInUser =
                    authenticationService.login(username, password);

                if (!loggedInUser) {
                    cout << "\nLogin failed. Check your username and password.\n";
                    continue;
                }

                cout << "\nLogin successful. Welcome, "
                     << loggedInUser->getFullName() << "!\n";

                int departmentId = 0;
                bool departmentAvailable = true;

                if (loggedInUser->getRole() == "Lecturer" ||
                    loggedInUser->getRole() == "HeadOfDepartment") {
                    departmentAvailable = getUserDepartment(
                        *loggedInUser,
                        userRepository,
                        departmentId);
                }

                bool loggedIn = true;

                while (loggedIn) {
                    showRoleMenu(*loggedInUser);

                    int roleChoice = readMenuChoice();

                    if (roleChoice == -1) {
                        cout << "\nInput ended. Exiting the application.\n";
                        return 0;
                    }

                    if (roleChoice == 0) {
                        cout << "\nYou have been logged out.\n";
                        loggedIn = false;
                        continue;
                    }

                    int highestOption =
                        getHighestRoleOption(*loggedInUser);

                    if (roleChoice < 1 ||
                        roleChoice > highestOption) {
                        cout << "\nInvalid menu choice. Try again.\n";
                        continue;
                    }

                    const string role = loggedInUser->getRole();

                    if (role == "Student") {
                        switch (roleChoice) {
                            case 1:
                                displayAvailableCourses(courseRepository);
                                break;

                            case 2:
                                enrollInCourse(
                                    *loggedInUser,
                                    enrollmentRepository,
                                    courseRepository);
                                break;

                            case 3:
                                displayEnrolledCourses(
                                    *loggedInUser,
                                    enrollmentRepository,
                                    courseRepository);
                                break;

                            case 4:
                                displayStudentAssignments(
                                    *loggedInUser,
                                    enrollmentRepository,
                                    assignmentRepository);
                                break;

                            case 5:
                                submitAssignment(
                                    *loggedInUser,
                                    enrollmentRepository,
                                    assignmentRepository,
                                    submissionRepository);
                                break;

                            case 6:
                                displayMySubmissions(
                                    *loggedInUser,
                                    submissionRepository);
                                break;

                            case 7:
                                displayMySubmissions(
                                    *loggedInUser,
                                    submissionRepository,
                                    true);
                                break;
                        }
                    }
                    else if (role == "Lecturer") {
                        if (!departmentAvailable) {
                            cout << "\nThis operation requires a valid department.\n";
                            continue;
                        }

                        switch (roleChoice) {
                            case 1:
                                displayDepartmentCourses(
                                    courseRepository,
                                    departmentId);
                                break;

                            case 2:
                                createAssignment(
                                    courseRepository,
                                    assignmentRepository,
                                    departmentId);
                                break;

                            case 3:
                                displayDepartmentSubmissions(
                                    submissionRepository,
                                    departmentId);
                                break;

                            case 4:
                                gradeSubmission(
                                    *loggedInUser,
                                    assignmentRepository,
                                    submissionRepository,
                                    departmentId);
                                break;

                            case 5:
                                grantIndividualExtension(
                                    *loggedInUser,
                                    extensionRepository);
                                break;
                        }
                    }
                    else if (role == "HeadOfDepartment") {
                        if (!departmentAvailable) {
                            cout << "\nThis operation requires a valid department.\n";
                            continue;
                        }

                        switch (roleChoice) {
                            case 1:
                                displaySubmissionsAwaitingModeration(
                                    submissionRepository,
                                    departmentId);
                                break;

                            case 2:
                                processHODSubmission(
                                    *loggedInUser,
                                    submissionRepository,
                                    departmentId,
                                    false);
                                break;

                            case 3:
                                processHODSubmission(
                                    *loggedInUser,
                                    submissionRepository,
                                    departmentId,
                                    true);
                                break;
                            case 4:
                                changeHODMark(
                                    *loggedInUser,
                                    assignmentRepository,
                                    submissionRepository,
                                    departmentId
                                );
                                break;
                        }
                    }
                    else if (role == "Administrator") {
                        switch (roleChoice) {
                            case 1:
                                manageUsers(userRepository, departmentRepository);
                                break;

                            case 2:
                                manageDepartments(departmentRepository);
                                break;

                            case 3:
                                manageCourses(courseRepository, departmentRepository);
                                break;

                            default:
                                cout << "Invalid Administrator menu choice.\n";
                                break;
                        }
                    }
                    else {
                        cout << "\nAccess denied: unrecognized user role.\n";
                        loggedIn = false;
                    }
                }
            }
            else {
                cout << "\nInvalid menu choice. Please select 1, 2, or 0.\n";
            }
        }
    }
    catch (const exception& error) {
        cerr << "\nApplication error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}