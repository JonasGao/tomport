cmake_minimum_required(VERSION 3.10)
project(tomport VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option to build tests
option(BUILD_TESTS "Build tests" ON)

# Create a library with the core functionality
add_library(tomport_core STATIC
    src/tomport_core.cpp
    src/tinyxml2.cpp
)
target_include_directories(tomport_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Main executable
add_executable(tomport 
    src/main.cpp
)
target_link_libraries(tomport tomport_core)

# Include directories
target_include_directories(tomport PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Install target
install(TARGETS tomport DESTINATION bin)

# Testing
if(BUILD_TESTS)
    enable_testing()
    
    # Download and configure Google Test
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/release-1.12.1.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    
    # Unit tests
    add_executable(tomport_unit_tests
        tests/unit_tests.cpp
    )
    target_link_libraries(tomport_unit_tests
        tomport_core
        GTest::gtest_main
    )
    
    # Integration tests
    add_executable(tomport_integration_tests
        tests/integration_tests.cpp
    )
    target_link_libraries(tomport_integration_tests
        tomport_core
        GTest::gtest_main
    )
    
    include(GoogleTest)
    gtest_discover_tests(tomport_unit_tests)
    gtest_discover_tests(tomport_integration_tests)
endif()
