
cmake_minimum_required(VERSION 3.20)

project(ModeratedLMS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SQLite3 REQUIRED)

add_executable(ModeratedLMS
    src/main.cpp
    src/models/User.cpp
    src/models/Student.cpp
    src/models/Lecturer.cpp
    src/models/HeadOfDepartment.cpp
    src/models/Administrator.cpp
    src/models/UserFactory.cpp
    src/models/Course.cpp
    src/models/Assignment.cpp
    src/models/Submission.cpp
    src/services/ThreeDayGraduatedLatePenaltyPolicy.cpp
    src/repositories/Database.cpp
    src/repositories/CourseRepository.cpp
)
find_package(SQLite3 REQUIRED)

target_include_directories(ModeratedLMS PRIVATE include)

target_link_libraries(ModeratedLMS PRIVATE SQLite::SQLite3)

target_compile_definitions(ModeratedLMS PRIVATE
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)

target_include_directories(ModeratedLMS PRIVATE
    include
)

target_link_libraries(ModeratedLMS PRIVATE
    SQLite::SQLite3
)

target_compile_definitions(
    ModeratedLMS
    PRIVATE PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)