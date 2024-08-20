#
# Adds option to generate unit tests using gtest or catkin test. Therefore, this
# macro is fully compliant with the catkin test compile options. In order to
# generate the tests manually, the CMake build flag ``BUILD_TEST`` must be set,
# e.g. ``-BUILD_TEST=ON``. The headers and sources can be defined outside as well
# as given as argument to the macro. It assumes that the gtest main function is
# given in test/src/utest.cpp, which can be altered using the ``GTEST_MAIN``
# argument and the project library is linked under ``${PROJECT_NAME}``. The
# variable SOURCE_DIR is defined to be used within test cases when refrering
# to e.g. config files located relative to the parent directory of the project.
#
# :param INCLUDE_PATH: Option to specify ``CMAKE_CURRENT_SOURCE_DIR``-relative
#   path of the include directory
# :type INCLUDE_PATH: string
# :param LINK_TARGET: Option to specify name of output executable (default utest)
# :type LINK_TARGET: string
# :param GTEST_MAIN: Option to specify ``CMAKE_CURRENT_SOURCE_DIR``-relative
#   path to the gtest main (default test/src/utest.cpp)
# :type GTEST_MAIN: string
# :param HEADERS: Option to specify ``CMAKE_CURRENT_SOURCE_DIR``-relative
#   header files
# :type HEADERS: list of strings
# :param SOURCES: Option to specify ``CMAKE_CURRENT_SOURCE_DIR``-relative
#   source files
# :type SOURCES: list of strings
#
# Example:
# ::
#
#   set(TEST_SOURCES
#     test_case1.cpp
#     ...
#   )
#
#   add_gtest_compile()
#
# @public
#
function(add_gtest_compile)
  cmake_parse_arguments(
    GTEST_COMPILE
    ""
    "INCLUDE_PATH;LINK_TARGET;GTEST_MAIN"
    "HEADERS;SOURCES"
    ${ARGN}
  )

  if(BUILD_TEST OR CATKIN_ENABLE_TESTING)
    message(STATUS "Building Tests Enabled")
    find_package(GTest QUIET)

    if(NOT DEFINED GTEST_COMPILE_LINK_TARGET)
      set(LINK_TARGET utest)
    else()
      set(LINK_TARGET ${GTEST_COMPILE_LINK_TARGET})
    endif()

    if(NOT DEFINED GTEST_COMPILE_GTEST_MAIN)
      set(GTEST_MAIN test/src/utest.cpp)
    else()
      set(GTEST_MAIN ${GTEST_COMPILE_GTEST_MAIN})
    endif()

    # define variable SOURCE_DIR to be used to find e.g. config files relative to the projects parent directory 
    add_definitions(-DSOURCE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\")
    #add_compile_definitions(-DSOURCE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\") (reguires CMake 3.12.4)

    ## Specify additional locations of test files
    if(DEFINED GTEST_COMPILE_HEADERS)
      list(APPEND TEST_HEADERS ${GTEST_COMPILE_HEADERS})
    endif()

    if(DEFINED GTEST_COMPILE_SOURCES)
      list(APPEND TEST_SOURCES ${GTEST_COMPILE_SOURCES})
    endif()

    ## define libraries to build
    if(DEFINED TEST_HEADERS AND DEFINED TEST_SOURCES)
      add_library(${PROJECT_NAME}-test ${TEST_SOURCES} ${TEST_HEADERS})
    elseif(DEFINED TEST_HEADERS)
      add_library(${PROJECT_NAME}-test ${TEST_HEADERS})
    elseif(DEFINED TEST_SOURCES)
      add_library(${PROJECT_NAME}-test ${TEST_SOURCES})
    else()
      add_library(${PROJECT_NAME}-test)
      set_target_properties(${PROJECT_NAME}-test PROPERTIES LINKER_LANGUAGE CXX)
    endif()
    
    target_link_libraries(${PROJECT_NAME}-test ${PROJECT_NAME} gtest gtest_main pthread)

    ## Add gtest based cpp test target and link libraries
    ## build catkin test suite
    if(CATKIN_ENABLE_TESTING)
      find_package(rostest REQUIRED)

      catkin_add_gtest(${LINK_TARGET} ${GTEST_MAIN} ${TEST_SOURCES})

      if(TARGET ${PROJECT_NAME}-test)
        if(DEFINED GTEST_COMPILE_INCLUDE_PATH)
          target_include_directories(${PROJECT_NAME}-test SYSTEM PUBLIC ${GTEST_COMPILE_INCLUDE_PATH})
        endif()
        target_link_libraries(${LINK_TARGET} ${PROJECT_NAME}-test)
      endif()
    ## build native test suite
    elseif(GTest_FOUND)
      enable_testing()

      add_executable(${LINK_TARGET} ${GTEST_MAIN} ${TEST_SOURCES})
      target_link_libraries(${LINK_TARGET} ${PROJECT_NAME}-test)
    elseif(NOT GTest_FOUND)
      message(WARNING "gtest not installed!")
    endif()
  else()
    message(STATUS "Building Tests Disabled")
  endif()
endfunction(add_gtest_compile)
