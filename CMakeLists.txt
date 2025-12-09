cmake_minimum_required(VERSION 3.10)
project(tomport VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add tinyxml2 as a subdirectory (we'll include it as a single header)
add_executable(tomport 
    src/main.cpp
    src/tinyxml2.cpp
)

# Include directories
target_include_directories(tomport PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Install target
install(TARGETS tomport DESTINATION bin)
