if(NOT DEFINED LINKER_SCRIPT)
message(FATAL_ERROR "No linker script defined")
endif()

message("Linker script: ${LINKER_SCRIPT}")

# Архитектурно зависимые флаги
set(GEN_ARCH_FLAGS "-mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
# Флаги предупреждений
set(GEN_WARN_FLAGS "-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wno-expansion-to-defined")
set(C_WARN_FLAGS   "${GEN_WARN_FLAGS}")
set(CXX_WARN_FLAGS "${GEN_WARN_FLAGS} -Wno-register -Wno-volatile -Wvla")
set(ASM_WARN_FLAGS "${GEN_WARN_FLAGS}")

# Общие флаги компиляции для Debug конфигурации
set(GEN_FLAGS_DEBUG "-O0 -g -DDEBUG=1")
# Общие флаги компиляции для Release конфигурации
set(GEN_FLAGS_RELEASE "-Os -DNDEBUG=1 -DRELEASE=1")
# Общие флаги компиляции для RelWithDebInfo конфигурации
set(GEN_FLAGS_RELDEB "-Og -g -DDEBUG=1")
# Общие флаги компиляции
set(GEN_FLAGS "${GEN_ARCH_FLAGS} -ffunction-sections -fdata-sections -funsigned-char -MMD -fomit-frame-pointer -fmessage-length=0 -fno-strict-aliasing")

set(CMAKE_C_FLAGS_DEBUG       "${GEN_FLAGS} ${GEN_FLAGS_DEBUG} ${C_WARN_FLAGS} -std=gnu11" CACHE INTERNAL "C Compiler debug options")
set(CMAKE_CXX_FLAGS_DEBUG     "${GEN_FLAGS} ${GEN_FLAGS_DEBUG} ${CXX_WARN_FLAGS} -std=gnu++17 -fno-rtti -fno-exceptions" CACHE INTERNAL "C++ Compiler debug options")
set(CMAKE_ASM_FLAGS_DEBUG     "${GEN_FLAGS} ${GEN_FLAGS_DEBUG} ${ASM_WARN_FLAGS} -x assembler-with-cpp" CACHE INTERNAL "ASM Compiler debug options")

set(CMAKE_C_FLAGS_RELWITHDEBINFO   "${GEN_FLAGS} ${GEN_FLAGS_RELDEB} ${C_WARN_FLAGS} -std=gnu11" CACHE INTERNAL "C Compiler debug options")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${GEN_FLAGS} ${GEN_FLAGS_RELDEB} ${CXX_WARN_FLAGS} -std=gnu++17 -fno-rtti -fno-exceptions" CACHE INTERNAL "C++ Compiler debug options")
set(CMAKE_ASM_FLAGS_RELWITHDEBINFO "${GEN_FLAGS} ${GEN_FLAGS_RELDEB} ${ASM_WARN_FLAGS} -x assembler-with-cpp" CACHE INTERNAL "ASM Compiler debug options")

set(CMAKE_C_FLAGS_RELEASE     "${GEN_FLAGS} ${GEN_FLAGS_RELEASE} ${C_WARN_FLAGS} -std=gnu11" CACHE INTERNAL "C Compiler release options")
set(CMAKE_CXX_FLAGS_RELEASE   "${GEN_FLAGS} ${GEN_FLAGS_RELEASE} ${CXX_WARN_FLAGS} -std=gnu++17 -fno-exceptions -fno-rtti" CACHE INTERNAL "C++ Compiler release options")
set(CMAKE_ASM_FLAGS_RELEASE   "${GEN_FLAGS} ${GEN_FLAGS_RELEASE} ${ASM_WARN_FLAGS} -x assembler-with-cpp" CACHE INTERNAL "ASM Compiler release options")

# Название используемых системных библиотек
set(LD_SYS_LIBS "-lc -lm -lnosys")

# Флаги линкера
set(CMAKE_C_LINK_FLAGS "--specs=nano.specs --specs=nosys.specs -Wl,--gc-sections ${GEN_FLAGS} ${LD_SYS_LIBS} -T${LINKER_SCRIPT} -Wl,-Map=${CMAKE_PROJECT_NAME}.map" CACHE INTERNAL "Linker options")