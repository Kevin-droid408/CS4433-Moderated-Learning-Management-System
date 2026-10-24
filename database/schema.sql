PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS departments (
    department_id INTEGER PRIMARY KEY AUTOINCREMENT,
    department_name TEXT NOT NULL UNIQUE
);

CREATE TABLE IF NOT EXISTS users (
    user_id INTEGER PRIMARY KEY AUTOINCREMENT,
    full_name TEXT NOT NULL,
    email TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,
    role TEXT NOT NULL CHECK (
        role IN (
            'Student',
            'Lecturer',
            'HeadOfDepartment',
            'Administrator'
        )
    ),
    department_id INTEGER,
    FOREIGN KEY (department_id)
        REFERENCES departments(department_id)
);

CREATE TABLE IF NOT EXISTS courses (
    course_id INTEGER PRIMARY KEY AUTOINCREMENT,
    course_code TEXT NOT NULL UNIQUE,
    course_name TEXT NOT NULL,
    department_id INTEGER NOT NULL,
    FOREIGN KEY (department_id)
        REFERENCES departments(department_id)
);

CREATE TABLE IF NOT EXISTS enrollments (
    student_id INTEGER NOT NULL,
    course_id INTEGER NOT NULL,
    PRIMARY KEY (student_id, course_id),
    FOREIGN KEY (student_id) REFERENCES users(user_id),
    FOREIGN KEY (course_id) REFERENCES courses(course_id)
);

CREATE TABLE IF NOT EXISTS assignments (
    assignment_id INTEGER PRIMARY KEY AUTOINCREMENT,
    course_id INTEGER NOT NULL,
    title TEXT NOT NULL,
    description TEXT NOT NULL,
    due_date TEXT NOT NULL,
    max_mark REAL NOT NULL CHECK (max_mark > 0),
    FOREIGN KEY (course_id) REFERENCES courses(course_id)
);

CREATE TABLE IF NOT EXISTS extensions (
    extension_id INTEGER PRIMARY KEY AUTOINCREMENT,
    assignment_id INTEGER NOT NULL,
    student_id INTEGER NOT NULL,
    new_due_date TEXT NOT NULL,
    approved_by INTEGER NOT NULL,
    reason TEXT NOT NULL,
    FOREIGN KEY (assignment_id)
        REFERENCES assignments(assignment_id),
    FOREIGN KEY (student_id) REFERENCES users(user_id),
    FOREIGN KEY (approved_by) REFERENCES users(user_id),
    UNIQUE (assignment_id, student_id)
);

CREATE TABLE IF NOT EXISTS submissions (
    submission_id INTEGER PRIMARY KEY AUTOINCREMENT,
    student_id INTEGER NOT NULL,
    assignment_id INTEGER NOT NULL,
    submitted_at TEXT NOT NULL,
    file_path TEXT NOT NULL,
    mark REAL,
    feedback TEXT NOT NULL DEFAULT '',
    status TEXT NOT NULL DEFAULT 'Submitted'
        CHECK (status IN (
            'Submitted',
            'Graded',
            'Moderated',
            'Released'
        )),
    FOREIGN KEY (student_id) REFERENCES users(user_id),
    FOREIGN KEY (assignment_id) REFERENCES assignments(assignment_id)
);

CREATE TABLE IF NOT EXISTS audit_logs (
    audit_id INTEGER PRIMARY KEY AUTOINCREMENT,
    submission_id INTEGER NOT NULL,
    changed_by INTEGER NOT NULL,
    changed_at TEXT NOT NULL,
    old_mark REAL,
    new_mark REAL NOT NULL,
    reason TEXT NOT NULL,
    FOREIGN KEY (submission_id)
        REFERENCES submissions(submission_id),
    FOREIGN KEY (changed_by) REFERENCES users(user_id)
);