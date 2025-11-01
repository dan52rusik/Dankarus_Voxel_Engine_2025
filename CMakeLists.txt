cmake_minimum_required(VERSION 3.20)
project(VoxelNoxel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

# Куда складывать артефакты сборки
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Список исходников (вся папка src рекурсивноо)
file(GLOB_RECURSE SRC_FILES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

add_executable(VoxelNoxel ${SRC_FILES})

# Если линкуешься со статическим GLEW (glew32s) — оставляем define.
# Если перейдёшь на динамический glew32.dll, убери эту строку.
target_compile_definitions(VoxelNoxel PRIVATE GLEW_STATIC)

# --- ЗАВИСИМОСТИ ---
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PNG REQUIRED)

# GLFW проще всего подтянуть через pkg-config (в MSYS2 есть файл glfw3.pc)
pkg_check_modules(GLFW3 REQUIRED glfw3)

# GLEW: берём инклуды и библиотеку (сначала пробуем статическую glew32s, затем glew32)
find_path(GLEW_INCLUDE_DIR GL/glew.h)
find_library(GLEW_LIBRARY NAMES glew32s glew32)

if(NOT GLEW_INCLUDE_DIR OR NOT GLEW_LIBRARY)
    message(FATAL_ERROR "GLEW not found. Install: pacman -S mingw-w64-x86_64-glew")
endif()

# ---- GLM (headers-only) ----
# В MSYS2 пакет glm ставит cmake-конфиг (glm::glm). Пробуем через CONFIG.
find_package(glm CONFIG QUIET)
if(glm_FOUND)
    target_link_libraries(VoxelNoxel PRIVATE glm::glm)
else()
    # Фолбэк на include-путь, если конфиг не найден
    find_path(GLM_INCLUDE_DIR glm/glm.hpp)
    if(NOT GLM_INCLUDE_DIR)
        message(FATAL_ERROR "GLM not found. Install: pacman -S mingw-w64-x86_64-glm")
    endif()
    target_include_directories(VoxelNoxel PRIVATE ${GLM_INCLUDE_DIR})
endif()

# Инклуды
target_include_directories(VoxelNoxel PRIVATE
    ${GLEW_INCLUDE_DIR}
    ${GLFW3_INCLUDE_DIRS}
)

# Линковка (порядок важен для MinGW)
target_link_libraries(VoxelNoxel PRIVATE
    ${GLEW_LIBRARY}
    ${GLFW3_LIBRARIES}
    OpenGL::GL
    PNG::PNG
)

# Применим флаги от pkg-config для GLFW (системные либы, defines и т.п.)
target_link_options(VoxelNoxel PRIVATE ${GLFW3_LDFLAGS_OTHER})
target_compile_options(VoxelNoxel PRIVATE ${GLFW3_CFLAGS_OTHER})

# Скопировать ресурсы (шейдеры/картинки) в build/bin/res
add_custom_command(TARGET VoxelNoxel POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/res
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/res
)
