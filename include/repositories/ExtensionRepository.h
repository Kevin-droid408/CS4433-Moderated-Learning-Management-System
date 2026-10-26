
#ifndef EXTENSION_REPOSITORY_H
#define EXTENSION_REPOSITORY_H

#include "Database.h"

#include <string>

class ExtensionRepository {
private:
    Database& database;

public:
    explicit ExtensionRepository(Database& database);

    // Grants or updates an individual student's extension.
    bool grantIndividualExtension(
        int assignmentId,
        int studentId,
        const std::string& newDueDate,
        int lecturerId,
        const std::string& reason
    );
};

#endif
