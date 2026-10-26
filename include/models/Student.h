#ifndef STUDENT_H
#define STUDENT_H

#include "User.h"
#include <string>

class Student : public User {
public:
    Student(
        int userId,
        const std::string& fullName,
        const std::string& email,
        const std::string& passwordHash
    );

    std::string getRole() const override;
};

#endif