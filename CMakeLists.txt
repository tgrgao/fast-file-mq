cmake_minimum_required(VERSION 3.14)
project(my_project)

# GoogleTest requires at least C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)



enable_testing()

add_executable(
  FileMQ-test
  test/FileMQ-test.cpp
  src/FileMQ.cpp
  src/QueueLock.cpp
  src/DataStorage.cpp
  src/MetadataStorage.cpp
  src/FileMQ.h
)
target_link_libraries(
  FileMQ-test
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(FileMQ-test)