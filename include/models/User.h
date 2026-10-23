
#ifndef USER_H
#define USER_H

#include <string>
using namespace std;

class User {
    private:
        int userId;
        string fullName;
        string email;
        string passwordHash;

    public:
        User(int userId,
            const string& fullName,
            const string& email,
            const string& passwordHash);

        virtual ~User();

        int getUserId() const;
        const string& getFullName() const;
        const string& getEmail() const;
        const string& getPasswordHash() const;

        virtual string getRole() const = 0;
};

#endif
