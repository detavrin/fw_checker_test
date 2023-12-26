add_library(syscore INTERFACE)

target_include_directories(syscore INTERFACE
	${CMAKE_CURRENT_LIST_DIR})

target_sources(syscore INTERFACE
	assert.c
	crc8_sw.c
	crc16_sw.c
	crc32_sw.c
	critical_section.c
	hex.c)
