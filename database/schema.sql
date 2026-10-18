-- Enable foreign key support in SQLite
PRAGMA foreign_keys = ON;

CREATE TABLE Departments (
    department_id INTEGER PRIMARY KEY AUTOINCREMENT,
    department_name TEXT NOT NULL UNIQUE
);

CREATE TABLE Users (
    user_id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE,
    email TEXT NOT NULL,
    password_hash TEXT NOT NULL,
    role TEXT NOT NULL CHECK(role IN ('Student', 'Lecturer', 'HeadOfDepartment', 'Administrator')),
    department_id INTEGER,
    FOREIGN KEY (department_id) REFERENCES Departments(department_id) ON DELETE SET NULL
);

CREATE TABLE Courses (
    course_id INTEGER PRIMARY KEY AUTOINCREMENT,
    course_name TEXT NOT NULL,
    department_id INTEGER NOT NULL,
    FOREIGN KEY (department_id) REFERENCES Departments(department_id) ON DELETE CASCADE
);

CREATE TABLE Enrollments (
    enrollment_id INTEGER PRIMARY KEY AUTOINCREMENT,
    course_id INTEGER NOT NULL,
    student_id INTEGER NOT NULL,
    FOREIGN KEY (course_id) REFERENCES Courses(course_id) ON DELETE CASCADE,
    FOREIGN KEY (student_id) REFERENCES Users(user_id) ON DELETE CASCADE,
    UNIQUE(course_id, student_id)
);

CREATE TABLE Assignments (
    assignment_id INTEGER PRIMARY KEY AUTOINCREMENT,
    course_id INTEGER NOT NULL,
    title TEXT NOT NULL,
    due_date DATETIME NOT NULL,
    max_mark REAL NOT NULL CHECK(max_mark > 0),
    allow_late BOOLEAN NOT NULL DEFAULT 1,
    FOREIGN KEY (course_id) REFERENCES Courses(course_id) ON DELETE CASCADE
);

CREATE TABLE Submissions (
    submission_id INTEGER PRIMARY KEY AUTOINCREMENT,
    assignment_id INTEGER NOT NULL,
    student_id INTEGER NOT NULL,
    raw_mark REAL CHECK(raw_mark >= 0),
    moderated_mark REAL CHECK(moderated_mark >= 0),
    status TEXT NOT NULL DEFAULT 'Submitted' CHECK(status IN ('Submitted', 'Late', 'Marked', 'Moderated', 'Released')),
    submitted_at DATETIME NOT NULL,
    FOREIGN KEY (assignment_id) REFERENCES Assignments(assignment_id) ON DELETE CASCADE,
    FOREIGN KEY (student_id) REFERENCES Users(user_id) ON DELETE CASCADE
);

CREATE TABLE AuditLogs (
    log_id INTEGER PRIMARY KEY AUTOINCREMENT,
    submission_id INTEGER NOT NULL,
    performed_by INTEGER NOT NULL,
    timestamp DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    old_value TEXT,
    new_value TEXT,
    reason TEXT NOT NULL,
    FOREIGN KEY (submission_id) REFERENCES Submissions(submission_id) ON DELETE CASCADE,
    FOREIGN KEY (performed_by) REFERENCES Users(user_id) ON DELETE RESTRICT
);