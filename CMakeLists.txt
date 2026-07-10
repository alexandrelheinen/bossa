cmake_minimum_required(VERSION 3.16)
project(bossa LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/final/bin")

include(FetchContent)

FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG 0.8.0)
set(YAML_CPP_BUILD_TESTS
    OFF
    CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS
    OFF
    CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(yaml-cpp)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND AND NOT CMAKE_CROSSCOMPILING)
  pkg_check_modules(SYSTEMD libsystemd)
endif()

set(BOSSA_CORE_SOURCES
    src/core/service.cpp
    src/core/config.cpp)

add_library(bossa_core STATIC ${BOSSA_CORE_SOURCES})
target_include_directories(bossa_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(bossa_core PUBLIC yaml-cpp)

if(SYSTEMD_FOUND)
  target_compile_definitions(bossa_core PUBLIC HAVE_SYSTEMD)
  target_link_libraries(bossa_core PUBLIC ${SYSTEMD_LIBRARIES})
  target_include_directories(bossa_core PUBLIC ${SYSTEMD_INCLUDE_DIRS})
endif()

add_executable(bossa src/daemon_main.cpp)
target_link_libraries(bossa PRIVATE bossa_core)

if(NOT CMAKE_CROSSCOMPILING)
  FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  set(gtest_force_shared_crt
      ON
      CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  enable_testing()

  add_executable(bossa_tests
      tests/core/service_test.cpp
      tests/core/config_test.cpp)

  target_link_libraries(bossa_tests
      PRIVATE
          bossa_core
          GTest::gtest_main)

  include(GoogleTest)
  gtest_discover_tests(bossa_tests)
endif()
