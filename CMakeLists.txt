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

# --- stb_image (header only, no CMake of its own) ----------------------------
FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        2c980bb59875b0d32144a71867fbdebb2f77cd20
    GIT_SHALLOW    FALSE)
FetchContent_MakeAvailable(stb)

# --- miniaudio (single header; loads the system audio backend at run time) ---
FetchContent_Declare(miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG        0.11.24
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(miniaudio)
find_package(Threads REQUIRED)

# --- cgltf (single header glTF loader) ----------------------------------------
FetchContent_Declare(cgltf
    GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
    GIT_TAG        v1.15
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(cgltf)

# --- Dear ImGui (no CMake of its own; build the needed sources) --------------
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.9
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw ${CMAKE_DL_LIBS})

# --- GLAD (vendored) --------------------------------------------------------
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)
target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})

# --- Core: simulation logic. Links only GLM, so GL/ImGui cannot leak in. ----
add_library(sim_core STATIC
    src/core/OrbitCamera.cpp
    src/core/ApproachController.cpp
    src/core/ShipAudio.cpp
    src/core/PulsarSim.cpp
    src/core/BlackHoleSim.cpp
    src/core/SupernovaSim.cpp
    src/core/MergerSim.cpp
    src/core/MagnetarSim.cpp
)
target_include_directories(sim_core PUBLIC src)
target_link_libraries(sim_core PUBLIC glm::glm)

# --- App --------------------------------------------------------------------
add_executable(spacesim
    src/main.cpp
    src/render/App.cpp
    src/render/Window.cpp
    src/render/Paths.cpp
    src/render/PostProcess.cpp
    src/render/ApproachFx.cpp
    src/render/AudioDevice.cpp
    src/render/CockpitRenderer.cpp
    src/render/ShipModel.cpp
    src/render/Primitives.cpp
    src/render/Shader.cpp
    src/render/ImGuiLayer.cpp
    src/render/MainMenu.cpp
    src/render/registry.cpp
    src/render/PulsarRenderer.cpp
    src/render/BlackHoleRenderer.cpp
    src/render/SupernovaRenderer.cpp
    src/render/MergerRenderer.cpp
    src/render/MagnetarRenderer.cpp
    src/render/Screenshot.cpp
    src/render/SkyTexture.cpp
    src/render/StbImage.cpp
)
target_include_directories(spacesim PRIVATE src ${stb_SOURCE_DIR} ${miniaudio_SOURCE_DIR} ${cgltf_SOURCE_DIR})
target_link_libraries(spacesim PRIVATE sim_core glfw glad glm::glm imgui Threads::Threads ${CMAKE_DL_LIBS})
target_compile_definitions(spacesim PRIVATE
    SHADER_SOURCE_DIR="${CMAKE_SOURCE_DIR}/shaders"
    ASSET_SOURCE_DIR="${CMAKE_SOURCE_DIR}/assets")

# Shaders are loaded at runtime from <exe dir>/shaders. The copy runs before every
# spacesim build, so IDEs that build only that target still get fresh shaders.
# The generator expression stops VS/Xcode from adding a per-config subfolder.
set_target_properties(spacesim PROPERTIES RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}>")
add_custom_target(copy_shaders
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_BINARY_DIR}/shaders)
add_dependencies(spacesim copy_shaders)

# Images/models are loaded at runtime from <exe dir>/assets.
add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets)
add_dependencies(spacesim copy_assets)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(spacesim PRIVATE -Wall -Wextra)
    set_source_files_properties(src/render/AudioDevice.cpp PROPERTIES COMPILE_OPTIONS "-w")
endif()

# --- Tests: core only, so no window or GPU is needed ---------------------------
include(CTest)
if(BUILD_TESTING)
    add_executable(core_tests tests/core_tests.cpp)
    target_link_libraries(core_tests PRIVATE sim_core)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(core_tests PRIVATE -Wall -Wextra)
    endif()
    add_test(NAME core_tests COMMAND core_tests)
endif()
