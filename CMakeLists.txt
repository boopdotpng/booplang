cmake_minimum_required(VERSION 3.20)
project(boop LANGUAGES C)

# set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug) # or Release if preferred
endif()

# define directories
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(BUILD_DIR ${CMAKE_SOURCE_DIR}/build)

# set output directories for binaries and object files
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BUILD_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BUILD_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BUILD_DIR})

# find all source files in src/
file(GLOB SRC_FILES "${SRC_DIR}/*.c")

# include headers
include_directories(${INCLUDE_DIR})

# create the executable
add_executable(boop ${SRC_FILES})

# enable/disable auto-formatting with a cmake option
option(AUTO_FORMAT "Enable auto-formatting before builds" ON)

# check if the format script exists and enable auto-formatting if AUTO_FORMAT is ON
if(AUTO_FORMAT)
    find_program(ASTYLE_EXECUTABLE astyle)
    if(ASTYLE_EXECUTABLE)
        message(STATUS "astyle found: ${ASTYLE_EXECUTABLE}")
        add_custom_target(
            format_code
            COMMAND ${CMAKE_SOURCE_DIR}/format.sh
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Auto-formatting source code..."
        )
        add_dependencies(boop format_code)
    else()
        message(WARNING "astyle not found. Skipping auto-formatting.")
    endif()
endif()

# hot-reloading: auto-rebuild when files change
add_custom_target(
    hot_reload
    COMMAND cmake --build ${CMAKE_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Hot-reloading changes..."
)

# optional: toggle auto-formatting
option(AUTO_FORMAT "Enable auto-formatting before builds" ON)
if(AUTO_FORMAT)
    add_dependencies(boop format_code)
endif()
