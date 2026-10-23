
#include "models/UserFactory.h"

#include <iostream>
#include <memory>
#include <stdexcept>

int main() {
    try {
        unique_ptr<User> user =
            UserFactory::createUser(
                "Student",
                1,
                "Student Example",
                "student@example.com",
                "test-hash");

        cout << "Name: " << user->getFullName() << '\n';
        cout << "Role: " << user->getRole() << '\n';

    } catch (const invalid_argument& error) {
        cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
