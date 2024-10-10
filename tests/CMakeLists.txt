cmake_minimum_required(VERSION 3.10)
project(MyCppWebServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories
include_directories(${PROJECT_SOURCE_DIR})

# Add source files
set(SOURCES
    server.cpp
    utils.cpp
)

# Add executable for server
add_executable(server ${SOURCES})

# Link libraries
find_package(OpenSSL REQUIRED)
target_link_libraries(server OpenSSL::SSL OpenSSL::Crypto pthread)

# Enable testing
enable_testing()

# Add Google Test
add_subdirectory(googletest)

# Include directories for Google Test
include_directories(googletest/googletest/include)
include_directories(googletest/googlemock/include)

# Add test executable
set(TEST_SOURCES
    tests/test_main.cpp
    tests/test_ends_with.cpp
    tests/test_get_mime_type.cpp
    tests/test_parse_request.cpp
)

add_executable(run_tests ${TEST_SOURCES} utils.cpp)

# Link Google Test libraries
target_link_libraries(run_tests gtest gtest_main pthread)

# Add tests
add_test(NAME RunAllTests COMMAND run_tests)
