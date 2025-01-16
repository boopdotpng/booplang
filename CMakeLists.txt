cmake_minimum_required(VERSION 3.20)
project(boop LANGUAGES C)

set(SUPPORTED_BUILD_TYPES "Debug" "Release" "RelWithDebInfo" "MinSizeRel")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type specified. Defaulting to Debug.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

if(CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE IN_LIST SUPPORTED_BUILD_TYPES)
    message(WARNING "Build type '${CMAKE_BUILD_TYPE}' is not supported. Falling back to Debug.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${SUPPORTED_BUILD_TYPES})

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(BUILD_DIR "${CMAKE_SOURCE_DIR}/build")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BUILD_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${BUILD_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${BUILD_DIR}")

file(GLOB SRC_FILES "${SRC_DIR}/*.c")

include_directories("${INCLUDE_DIR}")

add_executable(boop ${SRC_FILES})

option(AUTO_FORMAT "Enable auto-formatting before builds" ON)

if(AUTO_FORMAT)
    find_program(ASTYLE_EXECUTABLE astyle)
    if(ASTYLE_EXECUTABLE)
        message(STATUS "Found astyle at: ${ASTYLE_EXECUTABLE}")
        add_custom_target(
            format_code
            COMMAND "${CMAKE_SOURCE_DIR}/format.sh"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            COMMENT "Auto-formatting source code with astyle..."
        )
        add_dependencies(boop format_code)
    else()
        message(WARNING "astyle not found. Skipping auto-formatting.")
    endif()
endif()

add_custom_target(
    hot_reload
    COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Hot-reloading changes..."
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(boop PRIVATE DEBUG_MODE)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
endif()