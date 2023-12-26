add_library(board INTERFACE)

set(BOARD_DIR "${CMAKE_CURRENT_LIST_DIR}/${BOARD}")

# Check existence of the board root directory and board configuration file by its name
if(NOT EXISTS "${BOARD_DIR}")
    message(FATAL_ERROR "Not found boards/${BOARD} folder.")
endif()

target_include_directories(board INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(board INTERFACE "BOARD_NAME=${BOARD}")

add_subdirectory(${BOARD_DIR})