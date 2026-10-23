
#include "models/User.h"

User::User(int userId,
           const string& fullName,
           const string& email,
           const string& passwordHash)
    : userId(userId),
      fullName(fullName),
      email(email),
      passwordHash(passwordHash) {
}

User::~User() = default;

int User::getUserId() const {
    return userId;
}

const string& User::getFullName() const {
    return fullName;
}

const string& User::getEmail() const {
    return email;
}

const string& User::getPasswordHash() const {
    return passwordHash;
}
