#include "repositories/Database.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

Database::Database(const string& databasePath)
    : db(nullptr) {

    int result = sqlite3_open(databasePath.c_str(), &db);

    if (result != SQLITE_OK) {
        string errorMessage =
            db ? sqlite3_errmsg(db) : "Unable to allocate SQLite connection";

        if (db != nullptr) {
            sqlite3_close(db);
            db = nullptr;
        }

        throw runtime_error(
            "Failed to open database: " + errorMessage
        );
    }
}

Database::~Database() {
    if (db != nullptr) {
        sqlite3_close(db);
    }
}

void Database::execute(const string& sql) {
    char* errorMessage = nullptr;

    int result = sqlite3_exec(
        db,
        sql.c_str(),
        nullptr,
        nullptr,
        &errorMessage
    );

    if (result != SQLITE_OK) {
        string message = errorMessage
            ? errorMessage
            : "Unknown SQLite error";

        sqlite3_free(errorMessage);

        throw runtime_error(
            "SQL execution failed: " + message
        );
    }
}

sqlite3* Database::getConnection() const {
    return db;
}

void Database::initializeSchema(const std::string& schemaPath) {
    std::ifstream file(schemaPath);

    if (!file) {
        throw std::runtime_error(
            "Unable to open schema file: " + schemaPath
        );
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();

    execute(buffer.str());
}