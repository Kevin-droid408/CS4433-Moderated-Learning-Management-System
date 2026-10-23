
#include "models/HeadOfDepartment.h"

HeadOfDepartment::HeadOfDepartment(
    int userId,
    const string& fullName,
    const string& email,
    const string& passwordHash)
    : User(userId, fullName, email, passwordHash) {
}

string HeadOfDepartment::getRole() const {
    return "HeadOfDepartment";
}
