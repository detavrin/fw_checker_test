add_library(mcu_target INTERFACE)

if(${BOARD} STREQUAL "f373")
	set(MCU_FAMILY "STM32F3")
elseif(${BOARD} STREQUAL "nucleo_l476")
	set(MCU_FAMILY "STM32L4")
endif()

add_subdirectory(CMSIS)
add_subdirectory(${MCU_FAMILY})

target_link_libraries(mcu_target INTERFACE 
	cmsis-cortex-m 
	stm32-cube-fw)