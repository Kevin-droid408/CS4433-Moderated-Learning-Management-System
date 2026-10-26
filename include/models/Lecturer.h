#ifndef LECTURER_H
#define LECTURER_H

#include "User.h"
#include <string>

class Lecturer : public User {
public:
    Lecturer(
        int userId,
        const std::string& fullName,
        const std::string& email,
        const std::string& passwordHash
    );

    std::string getRole() const override;
};

#endif