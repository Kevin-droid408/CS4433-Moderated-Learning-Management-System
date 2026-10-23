
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
)

target_include_directories(ModeratedLMS PRIVATE
    include
)

target_link_libraries(ModeratedLMS PRIVATE
    SQLite::SQLite3
)
