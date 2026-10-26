
cmake_minimum_required(VERSION 3.20)

project(ModeratedLMS LANGUAGES CXX)

# --------------------------------------------------
# C++ Standard
# --------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --------------------------------------------------
# Find Required Libraries
# --------------------------------------------------
find_package(SQLite3 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED IMPORTED_TARGET libsodium)

# --------------------------------------------------
# Main Moderated LMS Application
# --------------------------------------------------
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
    src/services/CreateAccount.cpp
    
    # Repositories
    src/repositories/Database.cpp
    src/repositories/CourseRepository.cpp
    src/repositories/DepartmentRepository.cpp
    src/repositories/UserRepository.cpp
    src/repositories/AssignmentRepository.cpp
    src/repositories/SubmissionRepository.cpp
    src/repositories/EnrollmentRepository.cpp
    src/repositories/ExtensionRepository.cpp
)

target_include_directories(ModeratedLMS PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

target_compile_definitions(ModeratedLMS PRIVATE
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(ModeratedLMS PRIVATE
    SQLite3::SQLite3
    PkgConfig::SODIUM
)

# --------------------------------------------------
# Lecturer Account Creation Utility
# --------------------------------------------------
add_executable(CreateLecturer
    src/create_lecturer.cpp

    # Models
    src/models/User.cpp
    src/models/Student.cpp
    src/models/Lecturer.cpp
    src/models/HeadOfDepartment.cpp
    src/models/Administrator.cpp
    src/models/UserFactory.cpp

    # Repository
    src/repositories/Database.cpp
    src/repositories/UserRepository.cpp

    # Services
    src/services/PasswordService.cpp
)

target_include_directories(CreateLecturer PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(CreateLecturer PRIVATE
    SQLite3::SQLite3
    PkgConfig::SODIUM
)

# --------------------------------------------------
# Head of Department Account Creation Utility
# --------------------------------------------------
add_executable(CreateHOD
    src/CreateHOD.cpp

    # Models
    src/models/User.cpp
    src/models/Student.cpp
    src/models/Lecturer.cpp
    src/models/HeadOfDepartment.cpp
    src/models/Administrator.cpp
    src/models/UserFactory.cpp

    # Repository
    src/repositories/Database.cpp
    src/repositories/UserRepository.cpp

    # Services
    src/services/PasswordService.cpp
)

target_include_directories(CreateHOD PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(CreateHOD PRIVATE
    SQLite3::SQLite3
    PkgConfig::SODIUM
)

# --------------------------------------------------
# Department Setup Utility
# --------------------------------------------------
add_executable(SetupDepartments
    src/setup_departments.cpp

    # Models
    src/models/User.cpp
    src/models/Student.cpp
    src/models/Lecturer.cpp
    src/models/HeadOfDepartment.cpp
    src/models/Administrator.cpp
    src/models/UserFactory.cpp

    # Repositories
    src/repositories/Database.cpp
    src/repositories/UserRepository.cpp

    # Services
    src/services/PasswordService.cpp
)

target_include_directories(SetupDepartments PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

target_compile_definitions(SetupDepartments PRIVATE
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(SetupDepartments PRIVATE
    SQLite3::SQLite3
    PkgConfig::SODIUM
)
