set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_minimum_required(VERSION 3.29.0)

project(sshGame VERSION 0.0.0)
set(CMAKE_CXX_STANDARD 23)

find_package(Libssh REQUIRED)
find_package(Threads REQUIRED) # Needed for Libssh threads

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC 
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE ssh Threads::Threads)
