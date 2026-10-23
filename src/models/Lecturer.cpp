
#include "models/Lecturer.h"

Lecturer::Lecturer(int userId,
                   const string& fullName,
                   const string& email,
                   const string& passwordHash)
    : User(userId, fullName, email, passwordHash) {
}

string Lecturer::getRole() const {
    return "Lecturer";
}
