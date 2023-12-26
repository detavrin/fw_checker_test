cmake_minimum_required(VERSION 3.20)
project(fw_checker LANGUAGES C ASM)

message("Board name:   ${BOARD}")
message("Board folder: boards/${BOARD}")

add_subdirectory(targets)
add_subdirectory(source)
add_subdirectory(boards)
add_subdirectory(source/core)
add_subdirectory(source/dfu_host)

target_include_directories(app PUBLIC include)

target_link_libraries(app PUBLIC 
	mcu_target
	board
	syscore
	dfu_host)