
#include "models/UserFactory.h"

#include "models/Student.h"
#include "models/Lecturer.h"
#include "models/HeadOfDepartment.h"
#include "models/Administrator.h"

#include <stdexcept>

std::unique_ptr<User> UserFactory::createUser(
    const string& role,
    int userId,
    const string& fullName,
    const string& email,
    const string& passwordHash) {

    if (role == "Student") {
        return make_unique<Student>(
            userId, fullName, email, passwordHash);
    }

    if (role == "Lecturer") {
        return make_unique<Lecturer>(
            userId, fullName, email, passwordHash);
    }

    if (role == "HeadOfDepartment") {
        return make_unique<HeadOfDepartment>(
            userId, fullName, email, passwordHash);
    }

    if (role == "Administrator") {
        return make_unique<Administrator>(
            userId, fullName, email, passwordHash);
    }

    throw invalid_argument("Unknown user role: " + role);
}
