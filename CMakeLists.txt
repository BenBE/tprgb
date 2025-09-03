cmake_minimum_required (VERSION 3.16)

# Possible values: openocd, pyocd, stlink, blackmagic. stlink is default
# set(CMX_DEBUGGER "openocd")
# set(OPENOCD_CFG "${CMAKE_CURRENT_SOURCE_DIR}/openocd.cfg")

include(cubemx.cmake/arm-gcc.cmake)
include(cubemx.cmake/cubemx.cmake)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set(CMAKE_C_EXTENSIONS OFF) # Doesn't compile surprisingly, various issues
set(CMAKE_CXX_EXTENSIONS OFF)

project(tprgb LANGUAGES C CXX ASM)
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

add_executable(app app_version.c)
cubemx_target(
    TARGET app
    CUBEMX_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/app_stub"
    IOC "${CMAKE_CURRENT_LIST_DIR}/app_stub/app_stub.ioc"
    STARTUP "${CMAKE_CURRENT_LIST_DIR}/app_stub/startup_stm32f030f4px.s"
    LDSCRIPT "${CMAKE_CURRENT_LIST_DIR}/app_stub/custom_FLASH.ld"
    FLASH_TARGET_NAME flash_app
    IMG_ADDR 0x08002000
)
target_compile_options(app PRIVATE -Og -Wall -g -gdwarf-2)
target_compile_definitions(app PRIVATE STM32F030x6 USE_FULL_LL_DRIVER)
target_include_directories(app PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/app"
    "${CMAKE_CURRENT_LIST_DIR}/common"
)
file(GLOB_RECURSE APP_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/app/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/app/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/app/*.s"
    "${CMAKE_CURRENT_LIST_DIR}/common/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/*.cpp"
)
target_sources(app PRIVATE ${APP_SOURCES})

# Burn git commit hash to firmware file
add_custom_command(
    OUTPUT app_version.c _ # "_" is dummy output, so this command runs on every build
    COMMAND ${CMAKE_COMMAND}
        -D INPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/app/version.c.in
        -D OUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/app_version.c
        -P ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_version.cmake
)

add_executable(boot boot_version.c)
cubemx_target(
    TARGET boot
    CUBEMX_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/boot_stub"
    IOC "${CMAKE_CURRENT_LIST_DIR}/boot_stub/boot_stub.ioc"
    STARTUP "${CMAKE_CURRENT_LIST_DIR}/boot_stub/startup_stm32f030f4px.s"
    LDSCRIPT "${CMAKE_CURRENT_LIST_DIR}/boot_stub/custom_FLASH.ld"
    FLASH_TARGET_NAME flash_boot
    IMG_ADDR 0x08000000
    ELF2BIN_OPT "--pad-to 0x8002000 --gap-fill 0xff"
)
target_compile_options(boot PRIVATE -Og -Wall -g -gdwarf-2)
target_compile_definitions(boot PRIVATE STM32F030x6 USE_FULL_LL_DRIVER)
target_include_directories(boot PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/boot"
    "${CMAKE_CURRENT_LIST_DIR}/common"
)
file(GLOB_RECURSE BOOTLOADER_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/boot/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/boot/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/common/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/*.cpp"
)
target_sources(boot PRIVATE ${BOOTLOADER_SOURCES})

# Burn git commit hash to bootloader file
add_custom_command(
    OUTPUT boot_version.c _ # "_" is dummy output, so this command runs on every build
    COMMAND ${CMAKE_COMMAND}
        -D INPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/boot/version.c.in
        -D OUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/boot_version.c
        -P ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_version.cmake
)

add_custom_command(
    OUTPUT tprgb.bin
    COMMAND
    cat
    "$<TARGET_PROPERTY:boot,TARGET_FILE_BIN>"
    "$<TARGET_PROPERTY:app,TARGET_FILE_BIN>"
    > "tprgb.bin"
    DEPENDS app boot
    COMMENT "Generating firmware bundle"
)

add_custom_target(bundle ALL
    DEPENDS tprgb.bin
)

option(GCC_DEBUG_BUILD "Enable debugging information for GCC" ON)

if(GCC_DEBUG_BUILD)
    target_compile_options(app PRIVATE -Og -g -gdwarf-2)
    target_compile_options(boot PRIVATE -Og -g -gdwarf-2)
    message(STATUS "GCC debug build     : enabled")
else()
    target_compile_options(app PRIVATE -O3 -g)
    target_compile_options(boot PRIVATE -O3 -g)
    message(STATUS "GCC debug build     : disabled")
endif(GCC_DEBUG_BUILD)

option(WATCHDOG "Whether to use the hardware watchdog" ON)

if(WATCHDOG)
    target_compile_definitions(app PRIVATE ENABLE_WATCHDOG)
    message(STATUS "Hardware Watchdog   : enabled")
else()
    message(STATUS "Hardware Watchdog   : disabled")
endif(WATCHDOG)

#disable -Werror for now for not having to patch CubeMX files
set(OPT "-g -pedantic -Wall -Wextra -Wno-unused-parameter -Wduplicated-cond -Wduplicated-branches -Wunreachable-code -Wshadow -Winit-self -Wrestrict -Wnull-dereference -Wpointer-arith -Wcast-align -fno-strict-aliasing -Wfloat-equal -Wlogical-op -Wwrite-strings -Wformat=2 -Wno-error=expansion-to-defined -fno-omit-frame-pointer -fstack-usage -dP -flto-report")
#Had to disable: -Wstrict-overflow=5 -Wswitch-default -Wswitch-enum -Wconversion -Wundef -Wdouble-promotion

set(NO_EXC_RTTI "-fno-exceptions -fno-non-call-exceptions -fno-rtti")
set(NO_TEARDOWN "-fno-use-cxa-atexit")

add_custom_target(cleanall
    COMMAND ${CMAKE_COMMAND} --build . --target clean --config $<CONFIG>
)

add_custom_target(style
    COMMAND
    astyle -A3 -s4 -p -xg -n -r -j
    "${CMAKE_SOURCE_DIR}/app_stub/Src/*.c"
    "${CMAKE_SOURCE_DIR}/app_stub/Inc/*.h"
    "${CMAKE_SOURCE_DIR}/app/*.c"
    "${CMAKE_SOURCE_DIR}/app/*.h"
    "${CMAKE_SOURCE_DIR}/boot_stub/Src/*.c"
    "${CMAKE_SOURCE_DIR}/boot_stub/Inc/*.h"
    "${CMAKE_SOURCE_DIR}/boot/*.c"
    "${CMAKE_SOURCE_DIR}/boot/*.h"
    "${CMAKE_SOURCE_DIR}/common/*.c"
    "${CMAKE_SOURCE_DIR}/common/*.h"
#    "${CMAKE_SOURCE_DIR}/app/*.cpp"
#    "${CMAKE_SOURCE_DIR}/app/*.hpp"
    COMMAND
    find ${CMAKE_SOURCE_DIR}
    -path "${CMAKE_BINARY_DIR}/toolchain/*" -prune -o
    -path "${CMAKE_BINARY_DIR}/.*/*" -prune -o
    -path "${CMAKE_SOURCE_DIR}/.*/*" -prune -o
    -type f
    -regextype posix-egrep -regex ".*\.(c|h|ioc|project|xml|ld)"
    -exec dos2unix -q {} \;
    COMMAND
    LC_ALL=C.utf-8 sort -o
    "${CMAKE_SOURCE_DIR}/app_stub/app_stub.ioc"
    "${CMAKE_SOURCE_DIR}/app_stub/app_stub.ioc"
    COMMAND
    LC_ALL=C.utf-8 sort -o
    "${CMAKE_SOURCE_DIR}/boot_stub/boot_stub.ioc"
    "${CMAKE_SOURCE_DIR}/boot_stub/boot_stub.ioc"
    COMMENT "Formatting code"
    VERBATIM
)

# Print short per-section memory usage stats using fpv-gcc
add_custom_target(memusage
    COMMAND LC_ALL=C fpvgcc --ssec app.map | grep --color=none -P -- '-+-|TOTAL'
    COMMAND LC_ALL=C fpvgcc --sar app.map | grep --color=none -P -- '-+-|TOTAL'
    COMMAND LC_ALL=C fpvgcc --ssec boot.map | grep --color=none -P -- '-+-|TOTAL'
    COMMAND LC_ALL=C fpvgcc --sar boot.map | grep --color=none -P -- '-+-|TOTAL'
    DEPENDS app.elf boot.elf
    COMMENT "Print memory section usage information"
)

# Allow for direct flashing of the bundle
flash_target(bundle flash 0x08000000)
