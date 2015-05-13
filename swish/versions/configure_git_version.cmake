if(GIT_FOUND)

  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --long --dirty --always
    WORKING_DIRECTORY "${SOURCE_DIR}"
    RESULT_VARIABLE GIT_DESCRIBE_FAILURE
    ERROR_VARIABLE SWISH_GIT_ERROR
    OUTPUT_VARIABLE SWISH_GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(GIT_DESCRIBE_FAILURE)
    message(FATAL_ERROR "Git failed to describe version: ${SWISH_GIT_ERROR}")
  else()
    message("Git described version as ${SWISH_GIT_VERSION}")
  endif()

else()

  set(SWISH_GIT_VERSION "unknown")

endif()

configure_file(
  "${VERSION_CONFIGURE_INPUT}" "${VERSION_CONFIGURE_OUTPUT}" @ONLY)
