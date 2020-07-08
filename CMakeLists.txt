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

set(CMAKE_OBJCOPY aarch64-none-elf-objcopy)
set(OC_FLAGS --target elf64-littleaarch64 -O binary)

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
target_include_directories(${target} PRIVATE include .)

add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} ${OC_FLAGS} ${target} ${target_bin}
    COMMENT "Generating ${target_bin}"
)

add_subdirectory(kernel)
add_subdirectory(utils)
add_subdirectory(platform/${platform})
