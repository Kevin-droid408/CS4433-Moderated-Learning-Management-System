
#include "models/Student.h"

Student::Student(int userId,
                 const string& fullName,
                 const string& email,
                 const string& passwordHash)
    : User(userId, fullName, email, passwordHash) {
}

string Student::getRole() const {
    return "Student";
}
