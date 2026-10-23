
#ifndef LECTURER_H
#define LECTURER_H

#include "User.h"
#include <string>

class Lecturer : public User {
public:
    Lecturer(int userId,
             const string& fullName,
             const string& email,
             const string& passwordHash);

    string getRole() const override;
};

#endif
