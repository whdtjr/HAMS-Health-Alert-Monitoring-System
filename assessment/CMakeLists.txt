cmake_minimum_required(VERSION 3.20)

project(hams_assessment LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HAMS_BUILD_TESTS "Build unit tests" ON)

add_library(hams_assessment_core
    src/assessment/drowsiness_assessor.cpp
    src/assessment/hrv_fatigue_rule.cpp
    src/assessment/consecutive_detection_rule.cpp
    src/protocol/json_message_parser.cpp
)

target_include_directories(hams_assessment_core PUBLIC include)
target_compile_options(hams_assessment_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

if(UNIX)
    add_executable(hams_assessment_server
        src/main.cpp
        src/net/socket.cpp
        src/net/client_session.cpp
        src/net/tcp_listener.cpp
        src/server/assessment_server.cpp
        src/handlers/hrv_handler.cpp
        src/handlers/drowsiness_handler.cpp
        src/handlers/arrhythmia_handler.cpp
        src/alert/fifo_alert_publisher.cpp
    )

    target_link_libraries(hams_assessment_server
        PRIVATE hams_assessment_core pthread
    )
endif()

if(HAMS_BUILD_TESTS)
    enable_testing()

    add_executable(hams_core_tests
        tests/core_tests.cpp
    )

    target_link_libraries(hams_core_tests PRIVATE hams_assessment_core)
    add_test(NAME hams_core_tests COMMAND hams_core_tests)
endif()
