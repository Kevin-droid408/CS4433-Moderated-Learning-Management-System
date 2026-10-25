#include "repositories/Database.h"
#include "repositories/UserRepository.h"
#include "repositories/CourseRepository.h"
#include "repositories/AssignmentRepository.h"
#include "repositories/SubmissionRepository.h"

#include "models/User.h"
#include "models/Course.h"
#include "models/Assignment.h"
#include "models/Submission.h"

#include "services/AuthenticationService.h"

#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <exception>
#include <vector>
#include <fstream>
#include <ctime>
#include <cmath>

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

        if (cin.eof()) {
            return -1;
        }

        cout << "Invalid input. Please enter a number.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

/* Reads a required line of text. */
bool readRequiredText(const string& prompt, string& value) {
    while (true) {
        cout << prompt;

        if (!getline(cin, value)) {
            return false;
        }

        if (!value.empty() &&
            value.find_first_not_of(" \t\r\n") != string::npos) {
            return true;
        }

        cout << "This field cannot be empty. Please try again.\n";
    }
}

/* Reads a positive maximum mark when creating an assignment. */
double readPositiveMark() {
    double mark;

    while (true) {
        cout << "Enter maximum mark: ";

        if (cin >> mark) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            if (isfinite(mark) && mark > 0) {
                return mark;
            }

            cout << "Maximum mark must be a finite number greater than zero.\n";
        }
        else {
            if (cin.eof()) {
                return -1;
            }

            cout << "Invalid input. Enter a numeric mark.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }
}

/* Reads a mark between zero and the assignment's maximum mark. */
double readGradeMark(double maximumMark) {
    double mark;

    while (true) {
        cout << "Enter student's mark (0 to "
             << maximumMark << "): ";

        if (cin >> mark) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            if (isfinite(mark) &&
                mark >= 0 &&
                mark <= maximumMark) {
                return mark;
            }

            cout << "Invalid mark. Enter a value between 0 and "
                 << maximumMark << ".\n";
        }
        else {
            if (cin.eof()) {
                return -1;
            }

            cout << "Invalid input. Enter a numeric mark.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }
}

/* Creates a local timestamp for a new submission. */
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

/* Displays all available courses. */
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

/* Displays every assignment. */
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

/* Creates an assignment for an existing course. */
void createAssignment(
    CourseRepository& courseRepository,
    AssignmentRepository& assignmentRepository) {

    vector<Course> courses = courseRepository.getAllCourses();

    cout << "\n========== CREATE ASSIGNMENT ==========\n";

    if (courses.empty()) {
        cout << "No courses exist. Create a course first.\n";
        return;
    }

    for (const Course& course : courses) {
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

    bool courseExists = false;

    for (const Course& course : courses) {
        if (course.getCourseId() == courseId) {
            courseExists = true;
            break;
        }
    }

    if (!courseExists) {
        cout << "Invalid course ID.\n";
        return;
    }

    string title;
    string description;
    string dueDate;

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

    Assignment assignment(
        0, courseId, title, description, dueDate, maxMark
    );

    if (assignmentRepository.addAssignment(assignment)) {
        cout << "\nAssignment created successfully!\n";
    }
    else {
        cout << "\nAssignment could not be saved.\n";
    }
}

/* Allows the logged-in student to submit an assignment. */
void submitAssignment(
    const User& user,
    AssignmentRepository& assignmentRepository,
    SubmissionRepository& submissionRepository) {

    vector<Assignment> assignments =
        assignmentRepository.getAllAssignments();

    cout << "\n========== SUBMIT ASSIGNMENT ==========\n";

    if (assignments.empty()) {
        cout << "There are no assignments available to submit.\n";
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

    cout << "Enter the assignment ID to submit (0 to cancel).\n";
    int assignmentId = readMenuChoice();

    if (assignmentId <= 0) {
        cout << "Submission cancelled.\n";
        return;
    }

    bool assignmentExists = false;

    for (const Assignment& assignment : assignments) {
        if (assignment.getAssignmentId() == assignmentId) {
            assignmentExists = true;
            break;
        }
    }

    if (!assignmentExists) {
        cout << "Invalid assignment ID. No submission was saved.\n";
        return;
    }

    string filePath;

    if (!readRequiredText(
            "Enter the path to your submission file: ", filePath)) {
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
        0,
        user.getUserId(),
        assignmentId,
        getCurrentTimestamp(),
        filePath
    );

    if (submissionRepository.addSubmission(submission)) {
        cout << "\nAssignment submitted successfully!\n";
        cout << "Assignment ID: " << assignmentId << '\n';
        cout << "Status: Submitted\n";
    }
    else {
        cout << "\nThe submission could not be saved. "
             << "Check the assignment and database constraints.\n";
    }
}

/* Converts a submission status into readable text. */
string getSubmissionStatusText(SubmissionStatus status) {
    switch (status) {
        case SubmissionStatus::Submitted:
            return "Submitted";

        case SubmissionStatus::Graded:
            return "Graded";

        case SubmissionStatus::Moderated:
            return "Moderated";

        case SubmissionStatus::Released:
            return "Released";
    }

    return "Unknown";
}

/* Displays the logged-in student's submissions.
   Marks and feedback are visible only after release. */
void displayMySubmissions(
    const User& user,
    SubmissionRepository& submissionRepository) {

    vector<Submission> submissions =
        submissionRepository.getSubmissionsByStudent(user.getUserId());

    cout << "\n========== MY SUBMISSIONS ==========\n";

    if (submissions.empty()) {
        cout << "You have not submitted any assignments yet.\n";
        return;
    }

    for (const Submission& submission : submissions) {
        cout << "\nSubmission ID: " << submission.getSubmissionId()
             << "\nAssignment ID: " << submission.getAssignmentId()
             << "\nSubmitted at: " << submission.getSubmittedAt()
             << "\nStatus: "
             << getSubmissionStatusText(submission.getStatus());

        if (submission.getStatus() == SubmissionStatus::Released) {
            if (submission.getMark().has_value()) {
                cout << "\nMark: " << submission.getMark().value();
            }
            else {
                cout << "\nMark: Not available";
            }

            if (!submission.getFeedback().empty()) {
                cout << "\nFeedback: " << submission.getFeedback();
            }
        }
        else {
            cout << "\nMark: Not yet released";
            cout << "\nFeedback: Not yet released";
        }

        cout << "\n------------------------------------\n";
    }
}

/* Displays all submissions for lecturer review. */
void displayAllSubmissions(
    SubmissionRepository& submissionRepository) {

    vector<Submission> submissions =
        submissionRepository.getAllSubmissions();

    cout << "\n========== ALL STUDENT SUBMISSIONS ==========\n";

    if (submissions.empty()) {
        cout << "No student submissions have been received yet.\n";
        return;
    }

    cout << "Total submissions: " << submissions.size() << "\n";

    for (const Submission& submission : submissions) {
        cout << "\nSubmission ID: " << submission.getSubmissionId()
             << "\nStudent ID: " << submission.getStudentId()
             << "\nAssignment ID: " << submission.getAssignmentId()
             << "\nSubmitted at: " << submission.getSubmittedAt()
             << "\nFile path: " << submission.getFilePath()
             << "\nStatus: "
             << getSubmissionStatusText(submission.getStatus());

        if (submission.getMark().has_value()) {
            cout << "\nMark: " << submission.getMark().value();
        }
        else {
            cout << "\nMark: Not yet assigned";
        }

        if (!submission.getFeedback().empty()) {
            cout << "\nFeedback: " << submission.getFeedback();
        }

        cout << "\n---------------------------------------------\n";
    }
}

/* Allows a lecturer to grade a submission and record the reason.
   SubmissionRepository records the mark change in audit_logs. */
void gradeSubmission(
    const User& lecturer,
    AssignmentRepository& assignmentRepository,
    SubmissionRepository& submissionRepository) {

    vector<Submission> submissions =
        submissionRepository.getAllSubmissions();

    cout << "\n========== GRADE SUBMISSION ==========\n";

    if (submissions.empty()) {
        cout << "There are no submissions to grade.\n";
        return;
    }

    cout << "\nAvailable submissions:\n";

    for (const Submission& submission : submissions) {
        cout << "\nSubmission ID: " << submission.getSubmissionId()
             << "\nStudent ID: " << submission.getStudentId()
             << "\nAssignment ID: " << submission.getAssignmentId()
             << "\nSubmitted at: " << submission.getSubmittedAt()
             << "\nStatus: "
             << getSubmissionStatusText(submission.getStatus());

        if (submission.getStatus() == SubmissionStatus::Submitted ||
            submission.getStatus() == SubmissionStatus::Graded) {
            cout << "\nAvailable for grading";
        }
        else {
            cout << "\nNot available for grading";
        }

        cout << "\n--------------------------------------\n";
    }

    cout << "Enter the submission ID to grade (0 to cancel).\n";
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

    if (selectedSubmission == nullptr) {
        cout << "Invalid submission ID.\n";
        return;
    }

    if (selectedSubmission->getStatus() != SubmissionStatus::Submitted &&
        selectedSubmission->getStatus() != SubmissionStatus::Graded) {
        cout << "This submission cannot be graded because its status is "
             << getSubmissionStatusText(selectedSubmission->getStatus())
             << ".\n";
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
        cout << "The assignment associated with this submission "
             << "could not be found.\n";
        return;
    }

    cout << "\nAssignment: " << selectedAssignment->getTitle()
         << "\nMaximum mark: " << selectedAssignment->getMaxMark()
         << "\nCurrent status: "
         << getSubmissionStatusText(selectedSubmission->getStatus())
         << '\n';

    cout << "\nEnsure you have reviewed the submitted file before grading:\n"
         << selectedSubmission->getFilePath() << '\n';

    double mark = readGradeMark(selectedAssignment->getMaxMark());

    if (mark < 0) {
        cout << "Grading cancelled.\n";
        return;
    }

    string feedback;
    string reason;

    if (!readRequiredText("Enter feedback for the student: ", feedback)) {
        cout << "\nInput ended. Grading cancelled.\n";
        return;
    }

    if (!readRequiredText(
            "Enter the reason for this mark change: ", reason)) {
        cout << "\nInput ended. Grading cancelled.\n";
        return;
    }

    cout << "\nConfirm grading:\n"
         << "Submission ID: " << submissionId << '\n'
         << "Mark: " << mark << " / "
         << selectedAssignment->getMaxMark() << '\n'
         << "Feedback: " << feedback << '\n'
         << "Reason: " << reason << '\n'
         << "The submission will remain awaiting moderation.\n"
         << "1. Save mark\n"
         << "0. Cancel\n";

    int confirmation = readMenuChoice();

    if (confirmation != 1) {
        cout << "Grading cancelled. No changes were requested.\n";
        return;
    }

    bool saved = submissionRepository.gradeSubmission(
        submissionId,
        mark,
        feedback,
        lecturer.getUserId(),
        reason
    );

    if (saved) {
        cout << "\nMark saved successfully.\n";
        cout << "Audit record created for the mark change.\n";
        cout << "Submission status: Graded\n";
        cout << "The mark is not visible to the student until it is released.\n";
    }
    else {
        cout << "\nThe mark could not be saved.\n"
             << "Check the submission, mark, and grading status.\n";
    }
}

/*
    Displays submissions that are waiting for HOD moderation.
    Only submissions with status Graded are listed.
*/
void displaySubmissionsAwaitingModeration(
    SubmissionRepository& submissionRepository) {

    vector<Submission> submissions =
        submissionRepository.getAllSubmissions();

    cout << "\n========== SUBMISSIONS AWAITING MODERATION ==========\n";

    bool found = false;

    for (const Submission& submission : submissions) {
        if (submission.getStatus() == SubmissionStatus::Graded) {
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
                 << "\nStatus: Graded"
                 << "\n-----------------------------------------------\n";
        }
    }

    if (!found) {
        cout << "There are no submissions awaiting moderation.\n";
    }
}

/*
    Allows an authenticated HOD to moderate or release a submission.
    The caller selects the operation using releaseResult.
*/
void processHODSubmission(
    const User& hod,
    SubmissionRepository& submissionRepository,
    bool releaseResult) {

    // Defence in depth: do not rely only on the menu to enforce the role.
    if (hod.getRole() != "HeadOfDepartment") {
        cout << "Access denied. Only the Head of Department can perform this action.\n";
        return;
    }

    vector<Submission> submissions =
        submissionRepository.getAllSubmissions();

    const SubmissionStatus requiredStatus = releaseResult
        ? SubmissionStatus::Moderated
        : SubmissionStatus::Graded;

    cout << (releaseResult
        ? "\n========== RELEASE MODERATED RESULT ==========\n"
        : "\n========== MODERATE GRADED SUBMISSION ==========\n");

    bool found = false;

    for (const Submission& submission : submissions) {
        if (submission.getStatus() == requiredStatus) {
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
    }

    if (!found) {
        cout << (releaseResult
            ? "There are no moderated submissions awaiting release.\n"
            : "There are no graded submissions awaiting moderation.\n");
        return;
    }

    cout << "Enter the submission ID (0 to cancel): ";
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
        cout << "Invalid submission ID or submission status. No changes were made.\n";
        return;
    }

    string reason;

    if (!readRequiredText(
            releaseResult
                ? "Enter the reason for releasing this result: "
                : "Enter the moderation reason: ",
            reason)) {
        cout << "\nInput ended. Operation cancelled.\n";
        return;
    }

    cout << "\nConfirm operation:\n"
         << "Submission ID: " << submissionId
         << "\nCurrent status: "
         << getSubmissionStatusText(selectedSubmission->getStatus())
         << "\nNew status: " << (releaseResult ? "Released" : "Moderated")
         << "\nReason: " << reason
         << "\n1. Confirm\n"
         << "0. Cancel\n";

    int confirmation = readMenuChoice();

    if (confirmation != 1) {
        cout << "Operation cancelled. No changes were made.\n";
        return;
    }

    bool success = releaseResult
        ? submissionRepository.releaseSubmission(
              submissionId, hod.getUserId(), reason)
        : submissionRepository.moderateSubmission(
              submissionId, hod.getUserId(), reason);

    if (success) {
        cout << (releaseResult
            ? "\nResult released successfully. The student can now view the mark and feedback.\n"
            : "\nSubmission moderated successfully. It is now ready for release.\n");
    } else {
        cout << "\nThe operation could not be completed. Check the submission status and try again.\n";
    }
}

/* Displays the menu for the logged-in user's role. */
void showRoleMenu(const User& user) {
    cout << "\n========== " << user.getRole() << " MENU ==========\n";

    if (user.getRole() == "Student") {
        cout << "1. View available courses\n";
        cout << "2. View assignments\n";
        cout << "3. Submit assignment\n";
        cout << "4. View submission status\n";
        cout << "5. View released marks\n";
    }
    else if (user.getRole() == "Lecturer") {
        cout << "1. View my courses\n";
        cout << "2. Create assignment\n";
        cout << "3. View submissions\n";
        cout << "4. Grade submission\n";
    }
    else if (user.getRole() == "HeadOfDepartment") {
        cout << "1. View marks awaiting moderation\n";
        cout << "2. Moderate marks\n";
        cout << "3. Release marks\n";
    }
    else if (user.getRole() == "Administrator") {
        cout << "1. Manage users\n";
        cout << "2. Manage departments\n";
        cout << "3. Manage courses\n";
    }

    cout << "0. Log out\n";
    cout << "========================================\n";
}

int getHighestRoleOption(const User& user) {
    if (user.getRole() == "Student") {
        return 5;
    }
    else if (user.getRole() == "Lecturer") {
        return 4;
    }
    else if (user.getRole() == "HeadOfDepartment" ||
             user.getRole() == "Administrator") {
        return 3;
    }

    return -1;
}

int main() {
    try {
        Database database("database/lms.db");
        database.initializeSchema("database/schema.sql");

        UserRepository userRepository(database);
        CourseRepository courseRepository(database);
        AssignmentRepository assignmentRepository(database);
        SubmissionRepository submissionRepository(database);

        AuthenticationService authenticationService(userRepository);

        bool applicationRunning = true;

        while (applicationRunning) {
            cout << "\n========== MODERATED LMS ==========\n";
            cout << "1. Login\n";
            cout << "0. Exit\n";
            cout << "===================================\n";

            int mainChoice = readMenuChoice();

            if (mainChoice == -1 || mainChoice == 0) {
                applicationRunning = false;
                cout << "\nExiting the Moderated LMS. Goodbye!\n";
            }
            else if (mainChoice == 1) {
                string email;
                string password;

                if (!readRequiredText("Enter your email: ", email) ||
                    !readRequiredText("Enter your password: ", password)) {
                    cout << "\nInput ended. Exiting the application.\n";
                    return 0;
                }

                unique_ptr<User> loggedInUser =
                    authenticationService.login(email, password);

                if (!loggedInUser) {
                    cout << "\nLogin failed. Check your email and password.\n";
                    continue;
                }

                cout << "\nLogin successful. Welcome, "
                     << loggedInUser->getFullName() << "!\n";

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

                    int highestOption = getHighestRoleOption(*loggedInUser);

                    if (roleChoice < 1 || roleChoice > highestOption) {
                        cout << "\nInvalid menu choice. Try again.\n";
                        continue;
                    }

                    if (loggedInUser->getRole() == "Student") {
                        switch (roleChoice) {
                            case 1:
                                displayAvailableCourses(courseRepository);
                                break;

                            case 2:
                                displayAssignments(assignmentRepository);
                                break;

                            case 3:
                                submitAssignment(
                                    *loggedInUser,
                                    assignmentRepository,
                                    submissionRepository
                                );
                                break;

                            case 4:
                                displayMySubmissions(
                                    *loggedInUser,
                                    submissionRepository
                                );
                                break;

                            case 5:
                                cout << "\nReleased marks are displayed "
                                     << "under View submission status.\n";
                                displayMySubmissions(
                                    *loggedInUser,
                                    submissionRepository
                                );
                                break;
                        }
                    }
                    else if (loggedInUser->getRole() == "Lecturer") {
                        switch (roleChoice) {
                            case 1:
                                displayAvailableCourses(courseRepository);
                                break;

                            case 2:
                                createAssignment(
                                    courseRepository,
                                    assignmentRepository
                                );
                                break;

                            case 3:
                                displayAllSubmissions(submissionRepository);
                                break;

                            case 4:
                                gradeSubmission(
                                    *loggedInUser,
                                    assignmentRepository,
                                    submissionRepository
                                );
                                break;
                        }
                    }
                    else if (loggedInUser->getRole() == "HeadOfDepartment") {
                        switch (roleChoice) {
                            case 1:
                                displaySubmissionsAwaitingModeration(
                                    submissionRepository
                                );
                                break;

                            case 2:
                                processHODSubmission(
                                    *loggedInUser,
                                    submissionRepository,
                                    false
                                );
                                break;

                            case 3:
                                processHODSubmission(
                                    *loggedInUser,
                                    submissionRepository,
                                    true
                                );
                                break;
                        }
                    }
                    else {
                        cout << "\nThis feature has not been implemented yet.\n";
                    }
                }
            }
            else {
                cout << "\nInvalid menu choice. Please select 1 or 0.\n";
            }
        }
    }
    catch (const exception& error) {
        cerr << "\nApplication error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}