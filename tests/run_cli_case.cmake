# Exercise the real stdin/stdout executable; do not generate expected answers.
execute_process(COMMAND "${PROGRAM}" INPUT_FILE "${INPUT}"
    OUTPUT_VARIABLE actual ERROR_VARIABLE error RESULT_VARIABLE result TIMEOUT 60)
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "CLI failed for ${INPUT}: ${result}\n${error}")
endif()
if(NOT error STREQUAL "")
    message(FATAL_ERROR "Unexpected stderr: ${error}")
endif()
file(READ "${EXPECTED}" expected)
string(REPLACE "\r\n" "\n" actual "${actual}")
string(REPLACE "\r\n" "\n" expected "${expected}")
# Fixtures specify counts only. Check the CLI output shape, then compare its count line.
# The C++ checker below evaluates the actual returned witness on the input formula.
if(NOT actual MATCHES "^count [0-9]+\nwitness( none|( [01])*)\n$")
    message(FATAL_ERROR "Unexpected CLI output format for ${INPUT}:\n${actual}")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -E env
    "THREE_SAT_INPUT=${INPUT}" "THREE_SAT_EXPECTED=${EXPECTED}"
    "THREE_SAT_CLI_OUTPUT=${actual}" "${CHECKER}"
    OUTPUT_VARIABLE validation ERROR_VARIABLE validation_error RESULT_VARIABLE validation_result)
if(NOT validation_result STREQUAL "0")
    message(FATAL_ERROR "CLI witness check failed for ${INPUT}:\n${validation}\n${validation_error}")
endif()
string(REGEX MATCH "^count [0-9]+\n" actual_count "${actual}")
if(NOT actual_count STREQUAL expected)
    message(FATAL_ERROR "Output mismatch for ${INPUT}\nExpected:\n${expected}\nActual:\n${actual}")
endif()
