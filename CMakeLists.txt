
cmake_minimum_required(VERSION 3.20)

project(ModeratedLMS LANGUAGES CXX)

# --------------------------------------------------
# C++ Standard
# --------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --------------------------------------------------
# Code Coverage Configuration
# --------------------------------------------------
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)

if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(STATUS "Code coverage enabled")

        add_compile_options(
            --coverage
            -O0
            -g
        )

        add_link_options(
            --coverage
        )
    else()
        message(FATAL_ERROR
            "Code coverage is configured for GCC only."
        )
    endif()
endif()
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
# Administrator Account Creation Utility
# --------------------------------------------------
add_executable(CreateAdministrator
    src/CreateAdministrator.cpp

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

target_include_directories(CreateAdministrator PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

target_compile_definitions(CreateAdministrator PRIVATE
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(CreateAdministrator PRIVATE
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

# --------------------------------------------------
# Phase 5: GoogleTest Configuration
# --------------------------------------------------
include(CTest)

if(BUILD_TESTING)

    include(FetchContent)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
    )

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(googletest)

    include(GoogleTest)

    # --------------------------------------------------
    # Late Penalty Policy Unit Tests
    # --------------------------------------------------
    add_executable(LatePenaltyPolicyTests
        tests/LatePenaltyPolicyTest.cpp
        src/services/ThreeDayGraduatedLatePenaltyPolicy.cpp
    )

    target_include_directories(LatePenaltyPolicyTests PRIVATE
        "${CMAKE_SOURCE_DIR}/include"
    )

    target_link_libraries(LatePenaltyPolicyTests PRIVATE
        GTest::gtest_main
    )

    gtest_discover_tests(LatePenaltyPolicyTests)

    # --------------------------------------------------
    # Password Service Unit Tests
    # --------------------------------------------------
    add_executable(PasswordServiceTests
        tests/PasswordServiceTest.cpp
        src/services/PasswordService.cpp
    )

    target_include_directories(PasswordServiceTests PRIVATE
        "${CMAKE_SOURCE_DIR}/include"
    )

    target_link_libraries(PasswordServiceTests PRIVATE
        GTest::gtest_main
        PkgConfig::SODIUM
    )

    gtest_discover_tests(PasswordServiceTests)

    # --------------------------------------------------
    # Authentication Service Tests
    # --------------------------------------------------
    add_executable(AuthenticationServiceTests
        tests/AuthenticationServiceTest.cpp

        # Services
        src/services/AuthenticationService.cpp
        src/services/PasswordService.cpp

        # Repositories
        src/repositories/Database.cpp
        src/repositories/UserRepository.cpp

        # Models
        src/models/User.cpp
        src/models/Student.cpp
        src/models/Lecturer.cpp
        src/models/HeadOfDepartment.cpp
        src/models/Administrator.cpp
        src/models/UserFactory.cpp
    )

    target_include_directories(AuthenticationServiceTests PRIVATE
        "${CMAKE_SOURCE_DIR}/include"
    )

    target_link_libraries(AuthenticationServiceTests PRIVATE
        GTest::gtest_main
        SQLite3::SQLite3
        PkgConfig::SODIUM
    )

    gtest_discover_tests(AuthenticationServiceTests)

    # --------------------------------------------------
    # Submission Workflow Integration Tests
    # --------------------------------------------------
    add_executable(SubmissionWorkflowIntegrationTests
        tests/SubmissionWorkflowIntegrationTest.cpp

        # Models
        src/models/Assignment.cpp
        src/models/Submission.cpp

        # Repositories
        src/repositories/Database.cpp
        src/repositories/AssignmentRepository.cpp
        src/repositories/SubmissionRepository.cpp

        # Services
        src/services/ThreeDayGraduatedLatePenaltyPolicy.cpp
    )

    target_include_directories(SubmissionWorkflowIntegrationTests PRIVATE
        "${CMAKE_SOURCE_DIR}/include"
    )

    target_link_libraries(SubmissionWorkflowIntegrationTests PRIVATE
        GTest::gtest_main
        SQLite3::SQLite3
    )

    gtest_discover_tests(SubmissionWorkflowIntegrationTests)

    add_executable(CourseRepositoryTests
    tests/CourseRepositoryTest.cpp
    src/models/Course.cpp
    src/repositories/Database.cpp
    src/repositories/CourseRepository.cpp
    )

    target_include_directories(CourseRepositoryTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(CourseRepositoryTests PRIVATE
        GTest::gtest_main
        SQLite3::SQLite3
    )

    gtest_discover_tests(CourseRepositoryTests)

    add_executable(UserRepositoryTests
    tests/UserRepositoryTest.cpp
    src/models/User.cpp
    src/models/Student.cpp
    src/models/Lecturer.cpp
    src/models/HeadOfDepartment.cpp
    src/models/Administrator.cpp
    src/models/UserFactory.cpp
    src/repositories/Database.cpp
    src/repositories/UserRepository.cpp
    )

    target_include_directories(UserRepositoryTests PRIVATE include)

    target_link_libraries(UserRepositoryTests
        PRIVATE
        GTest::gtest_main
        SQLite3::SQLite3
    )

    include(GoogleTest)
    gtest_discover_tests(UserRepositoryTests)

    add_executable(DepartmentRepositoryTests
    tests/DepartmentRepositoryTest.cpp
    src/repositories/Database.cpp
    src/repositories/DepartmentRepository.cpp
    )

    target_include_directories(DepartmentRepositoryTests PRIVATE include)

    target_link_libraries(DepartmentRepositoryTests
        PRIVATE
        GTest::gtest_main
        SQLite3::SQLite3
    )

    include(GoogleTest)
    gtest_discover_tests(DepartmentRepositoryTests)

    add_executable(EnrollmentRepositoryTests
    tests/EnrollmentRepositoryTest.cpp
    src/repositories/EnrollmentRepository.cpp
    )

    target_include_directories(EnrollmentRepositoryTests PRIVATE
        include
    )

    target_link_libraries(EnrollmentRepositoryTests PRIVATE
        GTest::gtest_main
        SQLite3::SQLite3
    )

    include(GoogleTest)
    gtest_discover_tests(EnrollmentRepositoryTests)

    add_executable(ExtensionRepositoryTests
    tests/ExtensionRepositoryTest.cpp
    src/repositories/Database.cpp
    src/repositories/ExtensionRepository.cpp
    )

    target_include_directories(ExtensionRepositoryTests PRIVATE
        include
    )

    target_link_libraries(ExtensionRepositoryTests PRIVATE
        GTest::gtest_main
        SQLite3::SQLite3
    )

    include(GoogleTest)
    gtest_discover_tests(ExtensionRepositoryTests)

    add_executable(CreateAccountTests
    tests/CreateAccountTest.cpp
    src/services/CreateAccount.cpp
    src/services/PasswordService.cpp
    src/models/User.cpp
    src/models/Student.cpp
    src/models/Lecturer.cpp
    src/models/HeadOfDepartment.cpp
    src/models/Administrator.cpp
    src/models/UserFactory.cpp
    src/repositories/Database.cpp
    src/repositories/UserRepository.cpp
    )

    target_include_directories(CreateAccountTests PRIVATE include)

    target_link_libraries(CreateAccountTests PRIVATE
        GTest::gtest_main
        SQLite3::SQLite3
        sodium
    )

    include(GoogleTest)
    gtest_discover_tests(CreateAccountTests)

    add_executable(ThreeDayGraduatedLatePenaltyPolicyTests
    tests/ThreeDayGraduatedLatePenaltyPolicyTest.cpp
    src/services/ThreeDayGraduatedLatePenaltyPolicy.cpp
    )

    target_include_directories(ThreeDayGraduatedLatePenaltyPolicyTests PRIVATE include)

    target_link_libraries(ThreeDayGraduatedLatePenaltyPolicyTests PRIVATE
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(ThreeDayGraduatedLatePenaltyPolicyTests)    

endif()
