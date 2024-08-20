#
# Adds option to generate documenation using Doxygen. In order to
# generate the Doxygon, the CMake build flag ``BUILD_DOC`` must be set,
# e.g. ``-DBUILD_DOC=ON``.
#
# :param DOXYFILE_IN: Option to specify ``CMAKE_CURRENT_SOURCE_DIR``-relative
#   path to the Doxygen config file
# :type DOXYFILE_IN: string
# :param DOXYFILE_OUT: Option to specify ``CMAKE_CURRENT_BINARY_DIR``-relative
#   path to the Doxygen output file
# :type DOXYFILE_OUT: string
# :param DOXYGEN_NO_SOURCES: Option to specify if sources may be included in the
#   resulting doc
# :type DOXYGEN_NO_SOURCES: string
#
# Example:
# ::
#
#   add_doxygen_compile(
#     DOXYFILE_IN doc/Doxyfile.in
#     DOXYFILE_OUT Doxyfile
#   )
#
# @public
#
function(add_doxygen_compile)
  cmake_parse_arguments(
    DOXYGEN_COMPILE
    ""
    "DOXYFILE_IN;DOXYFILE_OUT"
    ""
    ${ARGN}
  )

  find_package(Doxygen QUIET)
  if(BUILD_DOC)
    if(DOXYGEN_FOUND)
      message(STATUS "Building Documentation Enabled")

      ## set input and output files
      if(DEFINED DOXYGEN_COMPILE_DOXYFILE_IN)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/${DOXYGEN_COMPILE_DOXYFILE_IN})
      else()
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in)
      endif()

      if(DEFINED DOXYGEN_COMPILE_DOXYFILE_OUT)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/${DOXYGEN_COMPILE_DOXYFILE_OUT})
      else()
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
      endif()
      
      ## check set compile flags 
      if(DOXYGEN_NO_SOURCES)
        set(VERBATIM_HEADERS "NO")
        set(SOURCE_BROWSER "NO")
      else()
        set(VERBATIM_HEADERS "YES")
        set(SOURCE_BROWSER "YES")
      endif()      

      ## request to configure the file
      configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

      ## note the option ALL which allows to build the docs together with the application
      add_custom_target(doc_doxygen ALL
        COMMAND ${CMAKE_COMMAND} -E echo_append "Building API Documentation..."
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E echo_append "API Documentation built in ${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)
    else()
      message(WARNING "Doxygen need to be installed to generate the doxygen documentation. Use 'sudo apt install --no-install-recommends doxygen' for minimal setup.")
    endif()
  else()
    message(STATUS "Building Documentation Disabled")
  endif()
endfunction(add_doxygen_compile)
