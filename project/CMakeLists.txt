# Set the minimum CMake version
cmake_minimum_required(VERSION 3.13)

# Set your project name
project(MySEALProject)

# SEAL requires C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the SEAL package.
# This is the crucial part. It will look for SEALConfig.cmake
# You will tell it where to look when you run the cmake command.
find_package(SEAL 4.1 REQUIRED)

# Add your executable (e.g., "my_app") from its source file ("main.cpp")
add_executable(my_app main.cpp filereader.cpp ckks_helper.cpp)

# Link your executable against the SEAL library
# This automatically handles all include paths and library files
target_link_libraries(my_app PRIVATE SEAL::seal)