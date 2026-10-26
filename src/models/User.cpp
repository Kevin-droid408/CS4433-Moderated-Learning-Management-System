
#include "models/User.h"

User::User(
    int userId,
    const std::string& fullName,
    const std::string& email,
    const std::string& passwordHash
)
    : userId(userId),
      username(""),
      fullName(fullName),
      email(email),
      passwordHash(passwordHash) {
}

User::~User() = default;

int User::getUserId() const {
    return userId;
}

const std::string& User::getUsername() const {
    return username;
}

const std::string& User::getFullName() const {
    return fullName;
}

const std::string& User::getEmail() const {
    return email;
}

const std::string& User::getPasswordHash() const {
    return passwordHash;
}

void User::setUsername(const std::string& username) {
    this->username = username;
}
