
#include "repositories/UserRepository.h"
#include "models/UserFactory.h"

#include <sqlite3.h>

#include <stdexcept>
#include <string>

UserRepository::UserRepository(Database& database)
    : database(database) {
}

bool UserRepository::addUser(
    const User& user,
    const std::string& username,
    const std::string& passwordHash,
    int departmentId) {

    if (username.empty() || passwordHash.empty()) {
        return false;
    }

    const char* sql =
        "INSERT INTO users "
        "(username, full_name, email, password_hash, role, department_id) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    sqlite3* connection = database.getConnection();
    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        connection, sql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection));
    }

    sqlite3_bind_text(
        statement, 1, username.c_str(), -1, SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement, 2, user.getFullName().c_str(), -1, SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement, 3, user.getEmail().c_str(), -1, SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement, 4, passwordHash.c_str(), -1, SQLITE_TRANSIENT
    );

    const std::string role = user.getRole();

    sqlite3_bind_text(
        statement, 5, role.c_str(), -1, SQLITE_TRANSIENT
    );

    if (departmentId > 0) {
        sqlite3_bind_int(statement, 6, departmentId);
    } else {
        sqlite3_bind_null(statement, 6);
    }

    result = sqlite3_step(statement);

    if (result == SQLITE_DONE) {
        sqlite3_finalize(statement);
        return true;
    }

    const int extendedResult = sqlite3_extended_errcode(connection);
    const std::string error = sqlite3_errmsg(connection);

    sqlite3_finalize(statement);

    if ((extendedResult & 0xFF) == SQLITE_CONSTRAINT) {
        return false;
    }

    throw std::runtime_error(error);
}

// Compatibility overload for existing setup utilities.
// Uses the part of the email before '@' as the initial username.
// Prefer the explicit-username overload for normal registration.
bool UserRepository::addUser(
    const User& user,
    const std::string& passwordHash,
    int departmentId) {

    const std::string& email = user.getEmail();
    const std::size_t atPosition = email.find('@');

    if (atPosition == std::string::npos || atPosition == 0) {
        return false;
    }

    const std::string username = email.substr(0, atPosition);

    return addUser(user, username, passwordHash, departmentId);
}

std::unique_ptr<User> UserRepository::findByUsername(
    const std::string& username) {

    if (username.empty()) {
        return nullptr;
    }

    const char* sql =
        "SELECT user_id, username, full_name, email, password_hash, role "
        "FROM users WHERE username = ? COLLATE NOCASE;";

    sqlite3* connection = database.getConnection();
    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        connection, sql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection));
    }

    sqlite3_bind_text(
        statement, 1, username.c_str(), -1, SQLITE_TRANSIENT
    );

    result = sqlite3_step(statement);

    if (result == SQLITE_DONE) {
        sqlite3_finalize(statement);
        return nullptr;
    }

    if (result != SQLITE_ROW) {
        std::string error = sqlite3_errmsg(connection);
        sqlite3_finalize(statement);
        throw std::runtime_error(error);
    }

    const int userId = sqlite3_column_int(statement, 0);

    auto readText = [statement](int column) -> std::string {
        const unsigned char* value = sqlite3_column_text(statement, column);
        return value
            ? reinterpret_cast<const char*>(value)
            : "";
    };

    const std::string savedUsername = readText(1);
    const std::string fullName = readText(2);
    const std::string email = readText(3);
    const std::string passwordHash = readText(4);
    const std::string role = readText(5);

    sqlite3_finalize(statement);

    std::unique_ptr<User> user = UserFactory::createUser(
        role, userId, fullName, email, passwordHash
    );

    user->setUsername(savedUsername);
    return user;
}

std::unique_ptr<User> UserRepository::findByEmail(
    const std::string& email) {

    const char* sql =
        "SELECT user_id, username, full_name, email, password_hash, role "
        "FROM users WHERE email = ?;";

    sqlite3* connection = database.getConnection();
    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        connection, sql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection));
    }

    sqlite3_bind_text(
        statement, 1, email.c_str(), -1, SQLITE_TRANSIENT
    );

    result = sqlite3_step(statement);

    if (result == SQLITE_DONE) {
        sqlite3_finalize(statement);
        return nullptr;
    }

    if (result != SQLITE_ROW) {
        std::string error = sqlite3_errmsg(connection);
        sqlite3_finalize(statement);
        throw std::runtime_error(error);
    }

    const int userId = sqlite3_column_int(statement, 0);

    auto readText = [statement](int column) -> std::string {
        const unsigned char* value = sqlite3_column_text(statement, column);
        return value
            ? reinterpret_cast<const char*>(value)
            : "";
    };

    const std::string savedUsername = readText(1);
    const std::string fullName = readText(2);
    const std::string savedEmail = readText(3);
    const std::string passwordHash = readText(4);
    const std::string role = readText(5);

    sqlite3_finalize(statement);

    std::unique_ptr<User> user = UserFactory::createUser(
        role, userId, fullName, savedEmail, passwordHash
    );

    user->setUsername(savedUsername);
    return user;
}

bool UserRepository::usernameExists(const std::string& username) {
    const char* sql =
        "SELECT 1 FROM users WHERE username = ? COLLATE NOCASE LIMIT 1;";

    sqlite3* connection = database.getConnection();
    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        connection, sql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection));
    }

    sqlite3_bind_text(
        statement, 1, username.c_str(), -1, SQLITE_TRANSIENT
    );

    result = sqlite3_step(statement);
    const bool exists = result == SQLITE_ROW;

    if (result != SQLITE_ROW && result != SQLITE_DONE) {
        std::string error = sqlite3_errmsg(connection);
        sqlite3_finalize(statement);
        throw std::runtime_error(error);
    }

    sqlite3_finalize(statement);
    return exists;
}

bool UserRepository::emailExists(const std::string& email) {
    const char* sql =
        "SELECT 1 FROM users WHERE email = ? LIMIT 1;";

    sqlite3* connection = database.getConnection();
    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        connection, sql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection));
    }

    sqlite3_bind_text(
        statement, 1, email.c_str(), -1, SQLITE_TRANSIENT
    );

    result = sqlite3_step(statement);
    const bool exists = result == SQLITE_ROW;

    if (result != SQLITE_ROW && result != SQLITE_DONE) {
        std::string error = sqlite3_errmsg(connection);
        sqlite3_finalize(statement);
        throw std::runtime_error(error);
    }

    sqlite3_finalize(statement);
    return exists;
}

std::optional<int> UserRepository::getDepartmentId(int userId) {
    const char* sql =
        "SELECT department_id FROM users WHERE user_id = ?;";

    sqlite3* connection = database.getConnection();
    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        connection, sql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection));
    }

    sqlite3_bind_int(statement, 1, userId);

    result = sqlite3_step(statement);

    if (result == SQLITE_ROW) {
        if (sqlite3_column_type(statement, 0) == SQLITE_NULL) {
            sqlite3_finalize(statement);
            return std::nullopt;
        }

        const int departmentId = sqlite3_column_int(statement, 0);
        sqlite3_finalize(statement);
        return departmentId;
    }

    if (result == SQLITE_DONE) {
        sqlite3_finalize(statement);
        return std::nullopt;
    }

    std::string error = sqlite3_errmsg(connection);
    sqlite3_finalize(statement);
    throw std::runtime_error(error);
}

std::vector<std::unique_ptr<User>> UserRepository::getAllUsers() {
    const char* sql =
        "SELECT user_id, username, full_name, email, password_hash, role "
        "FROM users ORDER BY user_id;";

    sqlite3* connection = database.getConnection();
    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        connection, sql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection));
    }

    std::vector<std::unique_ptr<User>> users;

    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        const int userId = sqlite3_column_int(statement, 0);

        auto readText = [statement](int column) -> std::string {
            const unsigned char* value =
                sqlite3_column_text(statement, column);

            return value
                ? reinterpret_cast<const char*>(value)
                : "";
        };

        const std::string username = readText(1);
        const std::string fullName = readText(2);
        const std::string email = readText(3);
        const std::string passwordHash = readText(4);
        const std::string role = readText(5);

        std::unique_ptr<User> user = UserFactory::createUser(
            role,
            userId,
            fullName,
            email,
            passwordHash
        );

        user->setUsername(username);

        users.push_back(std::move(user));
    }

    if (result != SQLITE_DONE) {
        const std::string error = sqlite3_errmsg(connection);
        sqlite3_finalize(statement);
        throw std::runtime_error(error);
    }

    sqlite3_finalize(statement);

    return users;
}