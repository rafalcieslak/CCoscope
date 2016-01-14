
FLEX_TARGET(tokens tokens.lex ${CMAKE_CURRENT_BINARY_DIR}/tokens.cpp)

file(GLOB SOURCES
  ./*.cpp
  )

message(${CMAKE_CURRENT_BINARY_DIR}/tokens.cpp)
message(${FLEX_tokens_OUTPUTS})

add_executable(
  ${CMAKE_PROJECT_NAME}
  ${SOURCES}
  ${FLEX_tokens_OUTPUTS}
  )

# Link target app with flex
target_link_libraries(
  ${CMAKE_PROJECT_NAME}
  fl
  )

# Copy the final binary to outside src dir
add_custom_command(
	TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${CMAKE_PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}" "${CMAKE_BINARY_DIR}/"
)