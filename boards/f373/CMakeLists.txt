target_compile_definitions(board INTERFACE "STM32F373xC")

target_sources(board INTERFACE 
	board.c
	stm32f3xx_it.c
	stm32f3xx_hal_msp.c)

target_include_directories(board INTERFACE ${CMAKE_CURRENT_LIST_DIR})