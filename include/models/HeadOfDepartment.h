
#ifndef HEAD_OF_DEPARTMENT_H
#define HEAD_OF_DEPARTMENT_H

#include "User.h"
#include <string>

class HeadOfDepartment : public User {
public:
    HeadOfDepartment(int userId,
                     const string& fullName,
                     const string& email,
                     const string& passwordHash);

    string getRole() const override;
};

#endif
