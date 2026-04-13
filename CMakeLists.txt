cmake_minimum_required(VERSION 3.20)
project(BioHasher)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Git REQUIRED)  #Required for versioning.

########################################
# The list of hash source files
########################################
include(hashes/Hashsrc.cmake)
set(HASH_SRC_FULL_PATHS ""
        lib/Specifics.h
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
add_library(BioHasherHashlib STATIC
        ${CMAKE_CURRENT_BINARY_DIR}/Hashrefs.cpp
        lib/Hashlib.cpp
        lib/HashInfo.cpp
        ${HASH_SRC_FULL_PATHS}
)
target_include_directories(BioHasherHashlib PUBLIC
        ${CMAKE_SOURCE_DIR}/lib
        ${CMAKE_SOURCE_DIR}/hashes
)



########################################
# Build all the tests
########################################
add_library(BioHasherTests STATIC
        lib/specifics.cpp
        lib/BioDataGeneration.cpp
        lib/LSHGlobals.cpp
        tests/LSHCollision.cpp
#        tests/LSHCollisionAndOrTest.cpp
#        tests/ApproxNearestNeighbourTest.cpp
)
target_include_directories(BioHasherTests PUBLIC
        ${CMAKE_SOURCE_DIR}/lib
        ${CMAKE_SOURCE_DIR}/tests
)
target_link_libraries(BioHasherTests PUBLIC BioHasherHashlib)

########################################
# Main executable
########################################
add_executable(BioHasher main.cpp)
target_link_libraries(BioHasher PRIVATE BioHasherTests)
target_include_directories(BioHasher PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/lib
        ${CMAKE_SOURCE_DIR}/hashes
        ${CMAKE_SOURCE_DIR}/tests
)

########################################
# Version stamping
########################################
add_custom_target(BioHasherVersion
        COMMAND ${CMAKE_COMMAND}
        -D SRC=${CMAKE_SOURCE_DIR}/version.h.in
        -D DST=${CMAKE_SOURCE_DIR}/version.h
        -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
        -P ${CMAKE_SOURCE_DIR}/version.cmake
)
add_dependencies(BioHasher BioHasherVersion)