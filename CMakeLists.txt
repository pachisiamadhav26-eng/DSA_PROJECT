cmake_minimum_required(VERSION 3.16)
project(leaderboard LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if(MSVC)
    add_compile_options(/W4 /UNDEBUG)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -UNDEBUG)
endif()

add_library(backends src/structA/avl.cpp src/structB/fenwick.cpp)
target_include_directories(backends PUBLIC ${CMAKE_SOURCE_DIR} include)

add_library(replay app/player_table.cpp app/replay.cpp)
target_include_directories(replay PUBLIC include app)

add_executable(leaderboard app/leaderboard.cpp)
target_link_libraries(leaderboard PRIVATE replay backends)

add_executable(oracle oracle/main.cpp)
target_include_directories(oracle PRIVATE include)

add_executable(differential oracle/differential.cpp)
target_link_libraries(differential PRIVATE backends)
target_include_directories(differential PRIVATE oracle)

add_executable(generator adversary/generator.cpp)

add_executable(bench_runner bench/runner.cpp)
target_compile_definitions(bench_runner PRIVATE BENCH_HAS_AVL BENCH_HAS_FENWICK BENCH_ORACLE BENCH_AVL_TOOLS BENCH_FENWICK_TOOLS)
target_include_directories(bench_runner PRIVATE oracle)
find_package(Threads REQUIRED)
target_link_libraries(bench_runner PRIVATE replay backends Threads::Threads)

if(WIN32)
    add_executable(leaderboard_web app/web/server.cpp)
    target_link_libraries(leaderboard_web PRIVATE replay backends ws2_32)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(player_table_test tests/player_table_test.cpp)
    target_link_libraries(player_table_test PRIVATE replay)

    add_executable(replay_test tests/replay_test.cpp)
    target_link_libraries(replay_test PRIVATE replay)

    add_executable(avl_test tests/avl_test.cpp)
    target_link_libraries(avl_test PRIVATE replay)
    target_include_directories(avl_test PRIVATE ${CMAKE_SOURCE_DIR})

    add_executable(fenwick_test tests/fenwick_test.cpp)
    target_link_libraries(fenwick_test PRIVATE backends)

    foreach(test player_table_test replay_test avl_test fenwick_test)
        add_test(NAME ${test} COMMAND ${test})
        set_tests_properties(${test} PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    endforeach()

    add_test(NAME differential_self COMMAND differential --self-test)
    find_package(Python3 3.8 REQUIRED COMPONENTS Interpreter)
    add_test(NAME integration_quick COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/test.py" --quick --bin-dir "${CMAKE_BINARY_DIR}/bin")
endif()
