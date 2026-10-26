#include "models/Lecturer.h"

Lecturer::Lecturer(
    int userId,
    const std::string& fullName,
    const std::string& email,
    const std::string& passwordHash
)
    : User(userId, fullName, email, passwordHash) {
}

std::string Lecturer::getRole() const {
    return "Lecturer";
}