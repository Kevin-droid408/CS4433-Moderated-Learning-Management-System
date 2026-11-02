# CS4433 – Moderated Learning Management System (LMS)

## 1. Project Overview

The **Moderated Learning Management System (LMS)** is a C++ application designed to manage academic activities within an institution. It provides functionality for managing users, departments, courses, student enrolments, assignments, submissions, grading, and moderation of academic results.

The system uses role-based access control to provide different functionality to four types of users:

- Student
- Lecturer
- Head of Department (HOD)
- Administrator

A key feature of the system is its moderated grading workflow, which allows lecturers to grade submissions, Heads of Department to review and moderate marks, and students to access their results after they have been released.

## 2. Main Features

### 2.1 User Authentication and Account Management

- User login and logout.
- Role-based access to system functionality.
- Password hashing and verification using libsodium.
- Account creation for Students, Lecturers, Heads of Department, and Administrators.
- User information stored in an SQLite database.

### 2.2 Student Features

Students can:

- View available courses.
- Enroll in courses.
- View their enrolled courses.
- View assignments available for their courses.
- Submit assignments.
- View submission statuses.
- View marks and feedback after results have been released.

### 2.3 Lecturer Features

Lecturers can:

- View courses within their department.
- Create assignments.
- View department submissions.
- Grade student submissions.
- Provide feedback.
- Grant individual extensions to students.

### 2.4 Head of Department Features

Heads of Department can:

- View marks awaiting moderation.
- Moderate marks awarded by lecturers.
- Adjust marks when necessary.
- Record reasons for mark changes.
- Release moderated results to students.

The moderation process provides an additional review stage before students receive their final results.

### 2.5 Administrator Features

Administrators can:

- View registered users.
- Create Student accounts.
- Create Lecturer accounts.
- Create Head of Department accounts.
- Create Administrator accounts.
- View and manage departments.
- Add departments.
- View and manage courses.
- Add courses and associate them with departments.

## 3. Assignment Submission and Late Penalty System

The system supports assignment submission and a graduated late penalty policy.

The existing `ThreeDayGraduatedLatePenaltyPolicy` applies the following rules:

| Submission time           |             Penalty |
| ------------------------- | ------------------: |
| On or before the deadline |                  0% |
| 1 day late                | 10% of maximum mark |
| 2 days late               | 20% of maximum mark |
| 3 days late               | 30% of maximum mark |
| More than 3 days late     |     Final mark is 0 |

Late submissions are accepted, and the applicable penalty is deducted from the awarded mark, subject to the policy.

Individual extensions can also be granted by lecturers, with a revised deadline and reason recorded in the database.

## 4. Moderated Grading Workflow

The grading process follows these stages:

1. **Submission:** A student submits an assignment.
2. **Grading:** A lecturer reviews the submission, awards a mark, and provides feedback.
3. **Moderation:** The Head of Department reviews the graded submission and may adjust the mark.
4. **Release:** The Head of Department releases the moderated result.
5. **Student Access:** The student can view the released mark and feedback.

Submission statuses are:

- `Submitted`
- `Graded`
- `Moderated`
- `Released`

This workflow helps ensure that results are reviewed before being made available to students.

## 5. Technologies Used

| Technology    | Purpose                                     |
| ------------- | ------------------------------------------- |
| C++17         | Main programming language                   |
| SQLite        | Relational database management              |
| CMake         | Build configuration and project compilation |
| libsodium     | Password hashing and verification           |
| VS Code       | Development environment                     |
| MSYS2 / MinGW | C++ compiler and build tools                |

## 6. Project Structure

The project follows a modular structure that separates models, repositories, services, and application entry points.

```text
CS4433-Moderated-Learning-Management-System/
│
├── CMakeLists.txt
├── README.md
│
├── database/
│   ├── schema.sql
│   └── lms.db
│
├── include/
│   ├── models/
│   │   ├── User.h
│   │   ├── Student.h
│   │   ├── Lecturer.h
│   │   ├── HeadOfDepartment.h
│   │   ├── Administrator.h
│   │   ├── UserFactory.h
│   │   ├── Course.h
│   │   ├── Assignment.h
│   │   └── Submission.h
│   │
│   ├── repositories/
│   │   ├── Database.h
│   │   ├── UserRepository.h
│   │   ├── CourseRepository.h
│   │   ├── DepartmentRepository.h
│   │   ├── EnrollmentRepository.h
│   │   ├── AssignmentRepository.h
│   │   ├── SubmissionRepository.h
│   │   └── ExtensionRepository.h
│   │
│   └── services/
│       ├── AuthenticationService.h
│       ├── PasswordService.h
│       ├── CreateAccount.h
│       ├── LatePenaltyPolicy.h
│       └── ThreeDayGraduatedLatePenaltyPolicy.h
│
├── src/
│   ├── main.cpp
│   │
│   ├── models/
│   ├── repositories/
│   ├── services/
│   │
│   ├── CreateAdministrator.cpp
│   ├── CreateHOD.cpp
│   ├── create_lecturer.cpp
│   └── setup_departments.cpp
│
└── build/
```

*The `build/` directory is generated during compilation. The database file is created and initialized by the application when necessary.*

## 7. Database Design

The application uses SQLite to store and manage system information.

The database schema includes the following main tables:

| Table              | Purpose                                                       |
| ------------------ | ------------------------------------------------------------- |
| `departments`      | Stores academic departments.                                  |
| `users`            | Stores user details, roles, and password hashes.              |
| `courses`          | Stores courses and their associated departments.              |
| `course_lecturers` | Associates lecturers with courses.                            |
| `enrollments`      | Records student course enrolments.                            |
| `assignments`      | Stores assignment information and deadlines.                  |
| `extensions`       | Records individual assignment extensions.                     |
| `submissions`      | Stores assignment submissions, marks, feedback, and statuses. |
| `audit_logs`       | Records changes to submission marks and reasons.              |

The database schema is defined in `database/schema.sql`.

## 8. System Requirements

Before building the project, ensure that the following are installed:

- C++17-compatible compiler.
- CMake 3.20 or later.
- SQLite3 development library.
- libsodium development library.
- MSYS2 / MinGW or another supported C++ toolchain.
- Git (optional, for version control).

## 9. Building the Project

### Step 1: Clone the repository

```bash
git clone https://github.com/Kevin-droid408/CS4433-Moderated-Learning-Management-System.git
```

Navigate into the project directory:

```bash
cd CS4433-Moderated-Learning-Management-System
```

### Step 2: Configure the project

Using a MinGW Makefiles generator:

```bash
cmake -S . -B build -G "MinGW Makefiles"
```

Ensure that CMake can locate SQLite3 and libsodium.

### Step 3: Compile the project

```bash
cmake --build build
```

This builds the main LMS application and the account/setup utility targets defined in `CMakeLists.txt`.

## 10. Running the Application

After a successful build, run the main application from the project root.

On Windows PowerShell:

```powershell
.\build\ModeratedLMS.exe
```

The application initializes the SQLite database using the schema in `database/schema.sql` and displays the login interface.

Users can log in using their registered account credentials.

## 11. Initial Administrator Setup

A first-time Administrator account can be created using the provided setup utility.

Run:

```powershell
.\build\CreateAdministrator.exe
```

Follow the prompts to enter and confirm the Administrator password.

The utility creates the initial account if one does not already exist.

The Administrator can then log in to manage users, departments, and courses.

## 12. Additional Setup Utilities

The project also defines the following executable utilities:

| Executable                | Purpose                                                       |
| ------------------------- | ------------------------------------------------------------- |
| `CreateAdministrator.exe` | Creates the initial Administrator account.                    |
| `CreateHOD.exe`           | Provides a utility for creating a Head of Department account. |
| `CreateLecturer.exe`      | Provides a utility for creating a Lecturer account.           |
| `SetupDepartments.exe`    | Provides department account and setup functionality.          |

These utilities are separate from the main LMS application.

## 13. Software Architecture

The system separates responsibilities into three main layers:

### Models

Represent the main entities in the application, including users, courses, assignments, and submissions.

### Repositories

Handle database operations such as inserting, retrieving, and updating records.

### Services

Implement application-level operations and business rules, including authentication, password security, account creation, and late penalty calculations.

This separation helps maintain the application and keeps database operations separate from business logic.

## 14. Project Objectives

The main objectives of the Moderated LMS are to:

- Centralise the management of academic users, departments, and courses.
- Support assignment creation and electronic submission.
- Apply a consistent late submission penalty policy.
- Provide a structured grading and moderation process.
- Ensure that students only access released results.
- Maintain records of important academic activities and mark changes.
- Enforce role-based access to system functionality.

## 15. Project Status

The project includes the core LMS functionality for student enrolment, assignment submission, grading, moderation, result release, and administration.

---

**Project:** CS4433 Moderated Learning Management System
**Language:** C++17
**Database:** SQLite
**Build System:** CMake
