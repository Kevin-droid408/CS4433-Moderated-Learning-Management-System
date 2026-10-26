
#ifndef USER_H
#define USER_H

#include <string>

class User {
    private:
        int userId;
        std::string username;
        std::string fullName;
        std::string email;
        std::string passwordHash;

    public:
        User(
            int userId,
            const std::string& fullName,
            const std::string& email,
            const std::string& passwordHash
        );

        virtual ~User();

        int getUserId() const;
        const std::string& getUsername() const;
        const std::string& getFullName() const;
        const std::string& getEmail() const;
        const std::string& getPasswordHash() const;

        void setUsername(const std::string& username);

        virtual std::string getRole() const = 0;
};

#endif
