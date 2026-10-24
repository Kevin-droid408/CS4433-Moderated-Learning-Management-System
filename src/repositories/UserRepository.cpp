
#include "repositories/UserRepository.h"
#include "models/UserFactory.h"

#include <sqlite3.h>
#include <stdexcept>

UserRepository::UserRepository(Database& database)
    : database(database) {
}

bool UserRepository::addUser(
    const User& user,
    const std::string& passwordHash,
    int departmentId) {

    const char* sql =
        "INSERT INTO users "
        "(full_name, email, password_hash, role, department_id) "
        "VALUES (?, ?, ?, ?, ?);";

    sqlite3* connection = database.getConnection();
    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        connection, sql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection));
    }

    // Parameter 1: full_name
    sqlite3_bind_text(
        statement, 1, user.getFullName().c_str(),
        -1, SQLITE_TRANSIENT
    );

    // Parameter 2: email
    sqlite3_bind_text(
        statement, 2, user.getEmail().c_str(),
        -1, SQLITE_TRANSIENT
    );

    // Parameter 3: password_hash
    sqlite3_bind_text(
        statement, 3, passwordHash.c_str(),
        -1, SQLITE_TRANSIENT
    );

    // Parameter 4: role
    const std::string role = user.getRole();

    sqlite3_bind_text(
        statement, 4, role.c_str(),
        -1, SQLITE_TRANSIENT
    );

    // Parameter 5: department_id
    if (departmentId > 0) {
        sqlite3_bind_int(statement, 5, departmentId);
    } else {
        sqlite3_bind_null(statement, 5);
    }

    result = sqlite3_step(statement);

    if (result == SQLITE_DONE) {
        sqlite3_finalize(statement);
        return true;
    }

    // Capture the error before finalizing the statement.
    const int extendedResult =
        sqlite3_extended_errcode(connection);

    const std::string error =
        sqlite3_errmsg(connection);

    sqlite3_finalize(statement);

    if ((extendedResult & 0xFF) == SQLITE_CONSTRAINT) {
        return false;
    }

    throw std::runtime_error(error);
}


std::unique_ptr<User> UserRepository::findByEmail(
    const std::string& email) {

    const char* sql =
        "SELECT user_id, full_name, email, password_hash, role "
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

    int userId = sqlite3_column_int(statement, 0);

    auto readText = [statement](int column) -> std::string {
        const unsigned char* value =
            sqlite3_column_text(statement, column);

        return value
            ? reinterpret_cast<const char*>(value)
            : "";
    };

    std::string fullName = readText(1);
    std::string savedEmail = readText(2);
    std::string passwordHash = readText(3);
    std::string role = readText(4);

    sqlite3_finalize(statement);

    return UserFactory::createUser(
        role,
        userId,
        fullName,
        savedEmail,
        passwordHash
    );
}
