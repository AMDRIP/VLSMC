# ============================================================================
# Библиотека ядра RAND Elecorner 36
# ============================================================================

set(KERNEL_HEADERS
    include/kernel/types.h
    include/kernel/event_bus.h
    include/kernel/syscalls.h
    include/kernel/process.h
    include/kernel/scheduler.h
    include/kernel/memory_manager.h
    include/kernel/filesystem.h
    include/kernel/fs_driver.h
    include/kernel/kbd_driver.h
    include/kernel/console_driver.h
    include/kernel/ipc.h
    include/kernel/io_manager.h
    include/kernel/user_manager.h
    include/kernel/app_runtime.h
    include/kernel/app_api.h
    include/kernel/logger.h
    include/kernel/system_monitor.h
    include/kernel/input_manager.h
    include/kernel/vga_driver.h
    include/kernel/kernel.h
)

set(KERNEL_SOURCES
    src/types.cpp
    src/event_bus.cpp
    src/syscalls.cpp
    src/process.cpp
    src/scheduler.cpp
    src/memory_manager.cpp
    src/filesystem.cpp
    src/fs_driver.cpp
    src/kbd_driver.cpp
    src/console_driver.cpp
    src/ipc.cpp
    src/io_manager.cpp
    src/user_manager.cpp
    src/app_runtime.cpp
    src/app_api.cpp
    src/logger.cpp
    src/system_monitor.cpp
    src/input_manager.cpp
    src/vga_driver.cpp
    src/kernel.cpp
)

add_library(re36_kernel STATIC
    ${KERNEL_HEADERS}
    ${KERNEL_SOURCES}
)

target_include_directories(re36_kernel
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(re36_kernel PUBLIC cxx_std_17)

# Псевдоним для удобства: re36::kernel
add_library(re36::kernel ALIAS re36_kernel)
