
#ifndef ADMINISTRATOR_H
#define ADMINISTRATOR_H

#include "User.h"
#include <string>
using namespace std;

class Administrator : public User {
    public:
        Administrator(int userId,
                    const string& fullName,
                    const string& email,
                    const string& passwordHash);

        string getRole() const override;
};

#endif
