
#include "services/PasswordService.h"

#include <sodium.h>
#include <stdexcept>

std::string PasswordService::hashPassword(
    const std::string& password) {

    if (password.empty()) {
        throw std::invalid_argument(
            "Password cannot be empty."
        );
    }

    if (sodium_init() < 0) {
        throw std::runtime_error(
            "Could not initialize libsodium."
        );
    }

    char hashedPassword[crypto_pwhash_STRBYTES];

    if (crypto_pwhash_str(
            hashedPassword,
            password.c_str(),
            static_cast<unsigned long long>(password.size()),
            crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {

        throw std::runtime_error(
            "Password hashing failed: insufficient resources."
        );
    }

    return std::string(hashedPassword);
}

bool PasswordService::verifyPassword(
    const std::string& password,
    const std::string& storedHash) {

    if (password.empty() || storedHash.empty()) {
        return false;
    }

    if (sodium_init() < 0) {
        throw std::runtime_error(
            "Could not initialize libsodium."
        );
    }

    return crypto_pwhash_str_verify(
        storedHash.c_str(),
        password.c_str(),
        static_cast<unsigned long long>(password.size())
    ) == 0;
}
