add_library(stm32-cube-fw INTERFACE)

file(GLOB LINKER_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/TOOLCHAIN_GCC/*.ld")

include(stm32l4)

target_include_directories(stm32-cube-fw INTERFACE
	${CMAKE_CURRENT_LIST_DIR}
	STM32L4xx_HAL_Driver/Inc
	STM32L4xx_HAL_Driver/Inc/Legacy
	CMSIS)

file(GLOB HAL_SOURCES "${CMAKE_CURRENT_LIST_DIR}/STM32L4xx_HAL_Driver/Src/*.c")

target_sources(stm32-cube-fw INTERFACE
	TOOLCHAIN_GCC/startup_stm32l476xx.s
	system_stm32l4xx.c
	${HAL_SOURCES})

target_compile_definitions(stm32-cube-fw INTERFACE "USE_HAL_DRIVER")