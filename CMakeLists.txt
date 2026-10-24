
cmake_minimum_required(VERSION 3.20)

project(ModeratedLMS LANGUAGES CXX)

# Use C++17.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required libraries.
find_package(SQLite3 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED IMPORTED_TARGET libsodium)

# Build the application.
add_executable(ModeratedLMS
    src/main.cpp

    # Models
    src/models/User.cpp
    src/models/Student.cpp
    src/models/Lecturer.cpp
    src/models/HeadOfDepartment.cpp
    src/models/Administrator.cpp
    src/models/UserFactory.cpp
    src/models/Course.cpp
    src/models/Assignment.cpp
    src/models/Submission.cpp

    # Services
    src/services/ThreeDayGraduatedLatePenaltyPolicy.cpp
    src/services/PasswordService.cpp
    src/services/AuthenticationService.cpp

    # Repositories
    src/repositories/Database.cpp
    src/repositories/CourseRepository.cpp
    src/repositories/DepartmentRepository.cpp
    src/repositories/UserRepository.cpp
    src/repositories/AssignmentRepository.cpp
)

# Locate project headers.
target_include_directories(ModeratedLMS PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

# Provide the absolute project directory to the application.
target_compile_definitions(ModeratedLMS PRIVATE
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)

# Link required libraries.
target_link_libraries(ModeratedLMS PRIVATE
    SQLite::SQLite3
    PkgConfig::SODIUM
)
