cmake_minimum_required(VERSION 3.12)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

project(arm-kernel)
set(target kernel.elf)
set(target_bin kernel.img)

set(CMAKE_C_COMPILER aarch64-none-elf-gcc)
set(CMAKE_CXX_COMPILER aarch64-none-elf-g++)

set(platform "virt" CACHE STRING "Select target platform")
string(TOUPPER ${platform} platform_def)

enable_language(ASM)
set(linker_script ${CMAKE_SOURCE_DIR}/linker.ld)

set(OC_FLAGS --target elf32-littlearm --set-section-flags .bss=contents,alloc,load -O binary)

add_executable(${target} "")

target_compile_features(${target} PRIVATE c_std_99)
target_compile_options(${target} PRIVATE
    -march=armv8-a
    -Werror
    -fno-builtin
    -ffreestanding
)
target_link_options(${target} PRIVATE
    -nostdlib
    -nostdinc
    -nodefaultlibs
    -nostartfiles
    -T ${linker_script}
)
target_compile_definitions(${target} PRIVATE ${platform_def})

target_include_directories(${target} PRIVATE include)

file(GLOB kernel_srcs CONFIGURE_DEPENDS "kernel/*.c" "kernel/*.s")
file(GLOB lib_srcs CONFIGURE_DEPENDS "lib/*.c" "lib/*.s")
file(GLOB platform_common_srcs CONFIGURE_DEPENDS "platform/*.c" "platform/*.s")
file(GLOB platform_srcs CONFIGURE_DEPENDS "platform/${platform}/*.c" "platform/${platform}/*.s")

target_sources(${target} PRIVATE ${kernel_srcs})
target_sources(${target} PRIVATE ${lib_srcs})
target_sources(${target} PRIVATE ${platform_common_srcs})
target_sources(${target} PRIVATE ${platform_srcs})

add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} ${OC_FLAGS} ${target} ${target_bin}
    COMMENT "Generating ${target_bin}"
)
