cmake_minimum_required(VERSION 3.20)
project(spacesim LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(FetchContent)

# --- GLFW -------------------------------------------------------------------
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

# Build the Wayland backend only when its dev packages exist; otherwise X11
# only (runs under XWayland on Wayland sessions).
if(UNIX AND NOT APPLE)
    find_package(PkgConfig QUIET)
    set(_glfw_wayland OFF)
    if(PkgConfig_FOUND)
        pkg_check_modules(_wl QUIET wayland-client wayland-egl wayland-cursor xkbcommon)
        find_program(_wl_scanner wayland-scanner)
        if(_wl_FOUND AND _wl_scanner AND EXISTS /usr/share/wayland-protocols)
            set(_glfw_wayland ON)
        endif()
    endif()
    set(GLFW_BUILD_WAYLAND ${_glfw_wayland} CACHE BOOL "" FORCE)
    message(STATUS "GLFW Wayland backend: ${_glfw_wayland}")
endif()

FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE)

# --- GLM --------------------------------------------------------------------
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE)

FetchContent_MakeAvailable(glfw glm)

# --- GLAD (vendored) --------------------------------------------------------
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)
target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})

# --- App --------------------------------------------------------------------
add_executable(spacesim
    src/main.cpp
    src/render/Window.cpp
)
target_include_directories(spacesim PRIVATE src)
target_link_libraries(spacesim PRIVATE glfw glad glm::glm)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(spacesim PRIVATE -Wall -Wextra)
endif()
