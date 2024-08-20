cmake_minimum_required(VERSION 3.1)
project(circular_lifo_buffer)

set (CMAKE_CXX_STANDARD 17)
#set(CMAKE_BUILD_TYPE Debug)
#Find catkin macros and libraries

find_package(catkin REQUIRED)

include_directories(include SYSTEM ${catkin_INCLUDE_DIRS})

# add cmake functions
list (APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include (add_doxygen_compile)
include (add_gtest_compile)

# add compile options
option(BUILD_TEST "Build tests" OFF)
option(BUILD_DOC "Build documentation" OFF)
option(BUILD_ALL "Build all" OFF)

if(BUILD_ALL)
  set(BUILD_TEST ON)
  set(BUILD_DOC ON)
endif()

# setup catkin package
catkin_package(
  INCLUDE_DIRS include
  LIBRARIES ${PROJECT_NAME}
)

###########
## Build ##
###########

include_directories(include SYSTEM)

set(HEADERS
    include/${PROJECT_NAME}/circular_lifo_buffer.h
)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${HEADERS})


#############
## Install ##
#############


## Mark executables and/or libraries for installation
install(TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

## Mark cpp header files for installation
install(DIRECTORY include/${PROJECT_NAME}/
  DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
  FILES_MATCHING PATTERN "*.h"
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


