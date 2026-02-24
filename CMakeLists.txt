cmake_minimum_required(VERSION 3.25)

project(RANDElecorner36
    VERSION      0.1.0
    DESCRIPTION  "RAND Elecorner 36 — Концептуальный симулятор ОС"
    LANGUAGES    CXX
)

# ============================================================================
# Стандарт C++
# ============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# Настройки сборки
# ============================================================================
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Каталог для бинарного вывода
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ============================================================================
# Опции
# ============================================================================
option(RE36_BUILD_TESTS   "Собрать юнит-тесты"         ON)
option(RE36_BUILD_GUI     "Собрать GUI (требует Qt 6)"  OFF)

# ============================================================================
# Предупреждения компилятора
# ============================================================================
if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ============================================================================
# Подпроекты
# ============================================================================
add_subdirectory(kernel)

if(RE36_BUILD_GUI)
    # add_subdirectory(gui)  # будет добавлено позже
endif()

if(RE36_BUILD_TESTS)
    enable_testing()
    # add_subdirectory(tests)  # будет добавлено позже
endif()

# ============================================================================
# Информация о сборке
# ============================================================================
message(STATUS "=== RAND Elecorner 36 ===")
message(STATUS "Версия:      ${PROJECT_VERSION}")
message(STATUS "C++ стандарт: ${CMAKE_CXX_STANDARD}")
message(STATUS "Компилятор:   ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Тесты:        ${RE36_BUILD_TESTS}")
message(STATUS "GUI:          ${RE36_BUILD_GUI}")
message(STATUS "=========================")
