cmake_minimum_required(VERSION 3.20)
project(cg_course LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Все программы складываются в build/bin, чтобы их было легко найти.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Комментарии в исходниках на русском — просим MSVC читать файлы как UTF-8.
if(MSVC)
    add_compile_options(/utf-8)
endif()

# --- Внешние библиотеки -------------------------------------------------------
# GLFW (окно + ввод) и GLM (математика) скачиваются при первой конфигурации.
include(FetchContent)

set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

FetchContent_Declare(glfw
    URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_Declare(glm
    URL https://github.com/g-truc/glm/archive/refs/tags/1.0.1.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(glfw glm)

# GLAD — загрузчик функций OpenGL 3.3 Core (сгенерирован заранее, лежит в external/).
add_library(glad STATIC external/glad/src/gl.c)
target_include_directories(glad PUBLIC external/glad/include)

# Общий набор зависимостей для всех лабораторных.
add_library(cg_deps INTERFACE)
target_link_libraries(cg_deps INTERFACE glad glfw glm::glm)

# --- Лабораторные по неделям --------------------------------------------------
add_subdirectory(week01)
add_subdirectory(week02)
add_subdirectory(week03)
