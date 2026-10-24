
#ifndef STUDENT_H
#define STUDENT_H

#include "User.h"
#include <string>

class Student : public User {
    public:
        Student(int userId,
                const string& fullName,
                const string& email,
                const string& passwordHash);

        string getRole() const override;
};

#endif
