cmake_minimum_required(VERSION 3.22)

project(mddl
    VERSION 0.1
    DESCRIPTION "MDDL Interpreter for MIDI-based programming"
    LANGUAGES CXX
    HOMEPAGE_URL "https://github.com/mshamas0020/mddl"
)



set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sources
set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(SOURCES
    ${SRC}/data_ref.cpp
    ${SRC}/data_ref.hpp
    ${SRC}/environment.cpp
    ${SRC}/environment.hpp
    ${SRC}/expr.cpp
    ${SRC}/expr.hpp
    ${SRC}/ief.hpp
    ${SRC}/interpreter.cpp
    ${SRC}/interpreter.hpp
    ${SRC}/main.cpp
    ${SRC}/midi_io.cpp
    ${SRC}/midi_io.hpp
    ${SRC}/operations.cpp
    ${SRC}/operations.hpp
    ${SRC}/printer.hpp
    ${SRC}/runtime.cpp
    ${SRC}/runtime.hpp
    ${SRC}/scheduler.cpp
    ${SRC}/scheduler.hpp
    ${SRC}/sequence.cpp
    ${SRC}/sequence.hpp
    ${SRC}/syntax.cpp
    ${SRC}/syntax.hpp
    ${SRC}/utils.hpp
)



# Submodules
set(SUBMODULES ${CMAKE_CURRENT_SOURCE_DIR}/submodules)

# libremidi
option(LIBREMIDI_NO_BOOST "" OFF)
add_subdirectory(${SUBMODULES}/libremidi)


# Target
set(TARGET mddl)
add_executable(${TARGET} ${SOURCES})
target_link_directories(${TARGET} PRIVATE ${SRC})
target_link_libraries(${TARGET} PRIVATE libremidi)

#### PROPERTIES ####
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${TARGET} PRIVATE "-Wall")
    target_compile_options(${TARGET} PRIVATE "-Wextra")
    target_link_options(${TARGET} PRIVATE "-static-libstdc++")
    target_link_options(${TARGET} PRIVATE "-static-libgcc") 
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${TARGET} PRIVATE "/W4")
endif()

# TODO Install