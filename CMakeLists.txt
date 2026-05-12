cmake_minimum_required(VERSION 3.20)
project(BioLSHasher)

# # Make SUBSEQUENCE_LENGTH configurable (TODO: REMOVE AFTER TESTING)
# set(SUBSEQUENCE_LENGTH 16 CACHE STRING "Subsequence length k parameter")
# message(STATUS "Building with SUBSEQUENCE_LENGTH=${SUBSEQUENCE_LENGTH}")

# # Make D_PARAM configurable (TODO: REMOVE AFTER TESTING)
# set(D_PARAM 16 CACHE STRING "Subsequence length k parameter")
# message(STATUS "Building with D_PARAM=${D_PARAM}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Git REQUIRED)  #Required for versioning.

########################################
# The list of hash source files
########################################
include(hashes/Hashsrc.cmake)
set(HASH_SRC_FULL_PATHS ""
        lib/specifics.h
        lib/specifics.cpp
#        hashes/minhashWithSignature.cpp
#        hashes/minhash.cpp
#        hashes/simhash.cpp
#        hashes/ssh1.cpp
#        hashes/ssh1.h
#        lib/BioDataMutationModels.cpp
#        lib/BioDataMutationModels.h
    )
foreach(hashsrc ${HASH_SRC_FILES})
    list(APPEND HASH_SRC_FULL_PATHS "${CMAKE_SOURCE_DIR}/${hashsrc}")
endforeach()

########################################
# Generate the list of USE_FAMILY() calls
########################################
add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Hashrefs.cpp
        DEPENDS
        ${CMAKE_SOURCE_DIR}/hashes/Hashrefs.cpp.in
        ${CMAKE_SOURCE_DIR}/hashes/Hashsrc.cmake
        ${HASH_SRC_FULL_PATHS}
        COMMAND ${CMAKE_COMMAND}
        -D SRCDIR=${CMAKE_SOURCE_DIR}
        -D SRC=${CMAKE_SOURCE_DIR}/hashes/Hashrefs.cpp.in
        -D DST=${CMAKE_CURRENT_BINARY_DIR}/Hashrefs.cpp
        -P ${CMAKE_SOURCE_DIR}/hashes/Hashrefs.cmake
        COMMENT "Generating Hashrefs.cpp to link hash implementations"
)

########################################
# Build all the hashes
########################################
add_library(BioLSHasherHashlib STATIC
        ${CMAKE_CURRENT_BINARY_DIR}/Hashrefs.cpp
        lib/Hashlib.cpp
        lib/HashInfo.cpp
        lib/Mathmult.cpp
        ${HASH_SRC_FULL_PATHS}
)

# Make SUBSEQUENCE_LENGTH configurable (TODO: REMOVE AFTER TESTING)
# target_compile_definitions(BioLSHasherHashlib PRIVATE 
#     SUBSEQUENCE_LENGTH=${SUBSEQUENCE_LENGTH}
# )
# # Make D_PARAM configurable (TODO: REMOVE AFTER TESTING)
# target_compile_definitions(BioLSHasherHashlib PRIVATE 
#     D_PARAM=${D_PARAM}
# )

target_include_directories(BioLSHasherHashlib PUBLIC
        ${CMAKE_SOURCE_DIR}/lib
        ${CMAKE_SOURCE_DIR}/hashes
)


########################################
# Threading availability detection
########################################

find_package( Threads )
if(NOT (MSVC))
    if(NOT ("${CMAKE_THREAD_LIBS_INIT}" STREQUAL ""))
        # Some Linux installations need the threading library linked
        # in as "whole-archive", or required symbols get missed,
        # leading to link failures. This shouldn't hurt to add if it
        # is not required on this system.
        whole_archive_link_flags("${CMAKE_THREAD_LIBS_INIT}" threads_link_command)
    endif()
endif()
if(Threads_FOUND)
    add_definitions(-DHAVE_THREADS)
endif()


########################################
# Build all the tests
########################################
add_library(BioLSHasherTests STATIC
        lib/specifics.cpp
        lib/BioDataGeneration.cpp
        lib/LSHGlobals.cpp
        tests/LSHCollision.cpp
        tests/ApproxNearestNeighbour.cpp
)
target_include_directories(BioLSHasherTests PUBLIC
        ${CMAKE_SOURCE_DIR}/lib
        ${CMAKE_SOURCE_DIR}/tests
)
target_link_libraries(BioLSHasherTests PUBLIC BioLSHasherHashlib)

########################################
# Main executable
########################################
add_executable(BioLSHasher main.cpp)
target_link_libraries(BioLSHasher PRIVATE BioLSHasherTests)
target_include_directories(BioLSHasher PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/lib
        ${CMAKE_SOURCE_DIR}/hashes
        ${CMAKE_SOURCE_DIR}/tests
)

########################################
# Version stamping
########################################
add_custom_target(BioLSHasherVersion
        COMMAND ${CMAKE_COMMAND}
        -D SRC=${CMAKE_SOURCE_DIR}/version.h.in
        -D DST=${CMAKE_SOURCE_DIR}/version.h
        -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
        -P ${CMAKE_SOURCE_DIR}/version.cmake
)
add_dependencies(BioLSHasher BioLSHasherVersion)