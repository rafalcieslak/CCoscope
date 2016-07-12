file(GLOB SOURCES
  ./*.cpp
  )

add_library(libcco STATIC
  ${SOURCES}
  )

set_target_properties(libcco PROPERTIES OUTPUT_NAME libcco)

# Disables the addition of one more "lib" prefix on libcco
SET_TARGET_PROPERTIES(libcco PROPERTIES PREFIX "")

# Copy the final lib to outside libcco dir
add_custom_command(
  TARGET libcco POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/libcco${CMAKE_STATIC_LIBRARY_SUFFIX}" "${CMAKE_BINARY_DIR}/"
  )
