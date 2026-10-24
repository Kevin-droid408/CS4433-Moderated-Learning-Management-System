
#ifndef PASSWORD_SERVICE_H
#define PASSWORD_SERVICE_H

#include <string>

class PasswordService {
    public:
    // Creates a salted Argon2id password hash.
        static std::string hashPassword(
            const std::string& password
        );

    // Checks a password against a stored hash.
        static bool verifyPassword(
            const std::string& password,
            const std::string& storedHash
        );
};

#endif
