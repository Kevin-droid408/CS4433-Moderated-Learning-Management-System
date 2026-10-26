#include "models/Student.h"

Student::Student(
    int userId,
    const std::string& fullName,
    const std::string& email,
    const std::string& passwordHash
)
    : User(userId, fullName, email, passwordHash) {
}

std::string Student::getRole() const {
    return "Student";
}