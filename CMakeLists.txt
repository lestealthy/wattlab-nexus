cmake_minimum_required(VERSION 3.16)
project(WattLabNexus VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# This CMake build targets the HOST only (tests + simulator). The firmware is
# built with the Arduino CLI; see BUILD.md.

include_directories(
    ${CMAKE_SOURCE_DIR}/firmware/include
    ${CMAKE_SOURCE_DIR}/host/simulator
    ${CMAKE_SOURCE_DIR}/host/mocks
)
# Hardware-independent firmware logic, shared by tests and the simulator.
set(FIRMWARE_SOURCES
    firmware/src/nexus_errors.cpp
    firmware/src/nexus_log.cpp
    firmware/src/nexus_config.cpp
    firmware/src/nexus_resource.cpp
    firmware/src/nexus_capture.cpp
    firmware/src/nexus_device_db.cpp
    firmware/src/nexus_test_engine.cpp
    firmware/src/nexus_decoders.cpp
    firmware/src/nexus_measurement.cpp
    firmware/src/nexus_calibration.cpp
    firmware/src/nexus_message.cpp
    firmware/src/nexus_crash.cpp
    firmware/src/nexus_time.cpp
    firmware/src/nexus_storage.cpp
    firmware/src/nexus_logger.cpp
    firmware/src/nexus_persist.cpp
    firmware/src/nexus_boot.cpp
    firmware/src/nexus_platform.cpp
)

set(SIMULATOR_SOURCES
    host/simulator/virtual_dut.cpp
    host/mocks/nexus_storage_host.cpp
)

set(TEST_SOURCES
    tests/unit/test_main.cpp
    tests/unit/test_config.cpp
    tests/unit/test_capture.cpp
    tests/unit/test_device_db.cpp
    tests/unit/test_test_engine.cpp
    tests/unit/test_resource.cpp
    tests/unit/test_decoders.cpp
    tests/unit/test_measurement.cpp
    tests/unit/test_message.cpp
    tests/unit/test_crash.cpp
    tests/unit/test_time.cpp
    tests/unit/test_storage.cpp
    tests/unit/test_logger.cpp
    tests/integration/test_integration.cpp
    tests/integration/test_alpha_flow.cpp
)

add_executable(test_runner ${TEST_SOURCES} ${FIRMWARE_SOURCES})
add_executable(simulator host/simulator/simulator_main.cpp ${SIMULATOR_SOURCES} ${FIRMWARE_SOURCES})

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_runner PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(simulator PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(NAME host_tests COMMAND test_runner)
