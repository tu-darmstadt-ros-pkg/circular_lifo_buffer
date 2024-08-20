cmake_minimum_required(VERSION 3.3)
project(circular_lifo_buffer)

set (CMAKE_CXX_STANDARD 17)
#set(CMAKE_BUILD_TYPE Debug)

## Find ament macros and libraries
find_package(ament_cmake REQUIRED)

include_directories(include SYSTEM)

# add cmake functions
list (APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include (add_doxygen_compile)
include (add_gtest_compile)

# add compile options
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

option(BUILD_TEST "Build tests" OFF)
option(BUILD_DOC "Build documentation" OFF)
option(BUILD_ALL "Build all" OFF)

if(BUILD_ALL)
  set(BUILD_TEST ON)
  set(BUILD_DOC ON)
endif()


###########
## Build ##
###########

## Specify header files
set(HEADERS
    include/${PROJECT_NAME}/circular_lifo_buffer.h
)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${HEADERS})

#############
## Install ##
#############

install(
  FILES include/circular_lifo_buffer/circular_lifo_buffer.h
  DESTINATION include
)

#############
## Testing ##
#############

set(TEST_SOURCES
    test/src/circular_lifo_buffer_tests.cpp
)

add_gtest_compile()

##########
## DOCS ##
##########

add_doxygen_compile()

ament_package()