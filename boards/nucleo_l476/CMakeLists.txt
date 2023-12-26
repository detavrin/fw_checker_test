target_compile_definitions(board INTERFACE 
	"STM32L476xx"
	"CLOCK_SOURCE=1") # MSI Clock source

target_sources(board INTERFACE 
	board.c
	system_clock.c
	stm32l4xx_it.c
	stm32l4xx_hal_msp.c)

target_include_directories(board INTERFACE ${CMAKE_CURRENT_LIST_DIR})