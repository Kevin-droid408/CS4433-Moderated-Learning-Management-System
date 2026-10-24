
#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>
using namespace std;

class Database {
    private:
        sqlite3* db;

    public:
    // Opens or creates the SQLite database.
        explicit Database(const string& databasePath);

    // Closes the database connection.
        ~Database();

    // Prevent copying the same database connection.
        Database(const Database&) = delete;
        Database& operator=(const Database&) = delete;

    // Executes SQL statements that do not return rows.
        void execute(const string& sql);

    // Loads and executes the SQL schema file.
        void initializeSchema(const string& schemaPath);

    // Provides access to the SQLite connection for repositories.
        sqlite3* getConnection() const;
};

#endif
