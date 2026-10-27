cmake_minimum_required(VERSION 3.16)
project(three_sat VERSION 0.1.0 LANGUAGES CXX)

add_library(three_sat STATIC
    src/assignment.cpp
    src/three_sat.cpp
    src/three_sat_solver.cpp
    src/three_sat_naive_solver.cpp
    src/three_sat_subexponential_solver.cpp
    src/detail/cnf.cpp
    src/detail/paper_polynomial.cpp
    src/detail/paper_design.cpp
    src/detail/paper_design_validation.cpp
    src/detail/paper_tensor_design.cpp
    src/detail/paper_schedule.cpp
    src/detail/paper_reduction.cpp
    src/detail/paper_pipeline.cpp
    src/detail/paper_counter.cpp
    src/three_sat_reader.cpp
)
target_include_directories(three_sat PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(three_sat PUBLIC cxx_std_17)
# Exact rational Taylor coefficients can grow far beyond machine integers.
# GMP is also available as mingw-w64-ucrt-x86_64-gmp on MSYS2.
find_path(GMP_INCLUDE_DIR gmpxx.h)
find_library(GMP_LIBRARY NAMES gmp)
find_library(GMPXX_LIBRARY NAMES gmpxx)
if(NOT GMP_INCLUDE_DIR OR NOT GMP_LIBRARY OR NOT GMPXX_LIBRARY)
    message(FATAL_ERROR "GMP C++ development files are required (MSYS2: mingw-w64-ucrt-x86_64-gmp).")
endif()
target_include_directories(three_sat PUBLIC "${GMP_INCLUDE_DIR}")
target_link_libraries(three_sat PUBLIC "${GMPXX_LIBRARY}" "${GMP_LIBRARY}")
set_target_properties(three_sat PROPERTIES CXX_EXTENSIONS OFF)

add_executable(three_sat_cli src/main.cpp)
set_target_properties(three_sat_cli PROPERTIES OUTPUT_NAME three_sat)
target_link_libraries(three_sat_cli PRIVATE three_sat)

add_executable(three_sat_example examples/main.cpp)
target_link_libraries(three_sat_example PRIVATE three_sat)

# Each implementation unit has a separately runnable test suite.
include(CTest)
if(BUILD_TESTING)
    # Pin GoogleTest 1.18.0, whose CMake settings support current CMake releases.
    include(FetchContent)
    if(POLICY CMP0135)
        cmake_policy(SET CMP0135 NEW)
    endif()
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    include(GoogleTest)
    foreach(suite IN ITEMS paper_pipeline paper_polynomial paper_design paper_design_validation paper_tensor_design paper_schedule paper_reduction paper_counter cnf assignment three_sat three_sat_solver three_sat_naive_solver three_sat_subexponential_solver three_sat_reader)
        add_executable(${suite}_tests tests/${suite}_tests.cpp)
        target_link_libraries(${suite}_tests PRIVATE three_sat GTest::gtest_main)
        gtest_discover_tests(${suite}_tests)
    endforeach()
    # Pair fixtures by relative path. A missing input or answer is a configuration error.
    add_executable(three_sat_solver_case_tests tests/three_sat_solver_case_tests.cpp)
    target_link_libraries(three_sat_solver_case_tests PRIVATE three_sat GTest::gtest_main)
    add_executable(cli_case_tests tests/cli_case_tests.cpp)
    target_link_libraries(cli_case_tests PRIVATE three_sat GTest::gtest_main)
    file(GLOB_RECURSE inputs CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/tests/cases/in"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/cases/in/*.txt")
    file(GLOB_RECURSE outputs CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/tests/cases/out"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/cases/out/*.txt")
    if(NOT inputs OR NOT "${inputs}" STREQUAL "${outputs}")
        message(FATAL_ERROR "tests/cases/in and out must contain matching, nonempty sets of .txt files")
    endif()
    foreach(case_file IN LISTS inputs)
        set(input "${CMAKE_CURRENT_SOURCE_DIR}/tests/cases/in/${case_file}")
        set(expected "${CMAKE_CURRENT_SOURCE_DIR}/tests/cases/out/${case_file}")
        add_test(NAME "SolverCase/${case_file}"
            COMMAND "${CMAKE_COMMAND}" -E env "THREE_SAT_INPUT=${input}" "THREE_SAT_EXPECTED=${expected}"
                "$<TARGET_FILE:three_sat_solver_case_tests>")
        add_test(NAME "CLI/${case_file}"
            COMMAND "${CMAKE_COMMAND}" "-DPROGRAM=$<TARGET_FILE:three_sat_cli>"
                "-DCHECKER=$<TARGET_FILE:cli_case_tests>"
                "-DINPUT=${input}" "-DEXPECTED=${expected}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_cli_case.cmake")
        set_tests_properties("SolverCase/${case_file}" "CLI/${case_file}" PROPERTIES
            LABELS "solver_cases" TIMEOUT 65)
    endforeach()
    # Smoke-test the executable entry point as well as the library APIs.
    add_test(NAME example_smoke COMMAND three_sat_example)
    set_tests_properties(example_smoke PROPERTIES
        PASS_REGULAR_EXPRESSION "Satisfying assignments: 6")
endif()




