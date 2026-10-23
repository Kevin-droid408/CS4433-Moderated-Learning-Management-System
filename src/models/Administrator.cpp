
#include "models/Administrator.h"

Administrator::Administrator(
    int userId,
    const string& fullName,
    const string& email,
    const string& passwordHash)
    : User(userId, fullName, email, passwordHash) {
}

string Administrator::getRole() const {
    return "Administrator";
}
