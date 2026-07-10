cmake_minimum_required(VERSION 3.16)
project(bossa LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/final/bin")

set(BOSSA_DAEMON_NAME bossa-daemon)

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

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3)
FetchContent_MakeAvailable(nlohmann_json)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND AND NOT CMAKE_CROSSCOMPILING)
  pkg_check_modules(SYSTEMD libsystemd)
  pkg_check_modules(GPIOD libgpiod)
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

set(BOSSA_IO_SOURCES
    src/io/linux_i2c_bus.cpp
    src/io/libgpiod_gpio.cpp)

add_library(bossa_io STATIC ${BOSSA_IO_SOURCES})
target_include_directories(bossa_io
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(bossa_io PUBLIC bossa_core)

if(GPIOD_FOUND)
  target_compile_definitions(bossa_io PUBLIC HAVE_LIBGPIOD)
  target_link_libraries(bossa_io PUBLIC ${GPIOD_LIBRARIES})
  target_include_directories(bossa_io PUBLIC ${GPIOD_INCLUDE_DIRS})
endif()

add_library(bossa_telemetry INTERFACE)
target_include_directories(bossa_telemetry
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(BOSSA_DRIVERS_SOURCES
    src/drivers/registry.cpp
    drivers/bme280/bme280_driver.cpp)

add_library(bossa_drivers STATIC ${BOSSA_DRIVERS_SOURCES})
target_include_directories(bossa_drivers
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/drivers/bme280)
target_link_libraries(bossa_drivers
    PUBLIC
        bossa_io
        bossa_telemetry
        nlohmann_json::nlohmann_json)

add_executable(${BOSSA_DAEMON_NAME} src/daemon_main.cpp)
target_link_libraries(${BOSSA_DAEMON_NAME}
    PRIVATE
        bossa_core
        bossa_drivers)

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
      tests/core/config_test.cpp
      tests/core/daemon_integration_test.cpp
      tests/drivers/bme280_driver_test.cpp
      tests/mocks/mock_gpio.cpp
      tests/mocks/mock_i2c_bus.cpp)

  target_include_directories(bossa_tests
      PRIVATE
          ${CMAKE_CURRENT_SOURCE_DIR}/tests/mocks
          ${CMAKE_CURRENT_SOURCE_DIR}/drivers/bme280)

  target_compile_definitions(bossa_tests
      PRIVATE
          BOSSA_DAEMON_BINARY="$<TARGET_FILE:${BOSSA_DAEMON_NAME}>")

  add_dependencies(bossa_tests ${BOSSA_DAEMON_NAME})

  target_link_libraries(bossa_tests
      PRIVATE
          bossa_core
          bossa_drivers
          GTest::gtest_main)

  include(GoogleTest)
  gtest_discover_tests(bossa_tests)
endif()
