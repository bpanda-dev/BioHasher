execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --always
        OUTPUT_VARIABLE BIOHASHER_GIT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE GIT_RESULT
)

if(NOT GIT_RESULT EQUAL 0)
    set(BIOHASHER_GIT_VERSION "unknown")
endif()

configure_file(${SRC} ${DST})