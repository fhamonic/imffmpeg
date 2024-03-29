cmake_minimum_required(VERSION 3.12)

project(
    ImFFmpeg
    VERSION 1.0
    DESCRIPTION "ImFFmpeg"
    LANGUAGES CXX)

# set(CMAKE_VERBOSE_MAKEFILE ON)

# ################### Options ####################
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CONAN_DISABLE_CHECK_COMPILER ON)

option(WARNINGS "" OFF)
option(HARDCORE_WARNINGS "" OFF)
option(COMPILE_FOR_NATIVE "" OFF)
option(COMPILE_WITH_LTO "" OFF)

set(WARNINGS_LIST "-Wall;-Wextra;")
set(HARDCORE_WARNINGS_LIST
    "-Wall;-Wextra;-Wshadow;-Wpedantic;-Wnon-virtual-dtor;-Wold-style-cast;-Wcast-align;-Wunused;-Woverloaded-virtual;-Wpedantic;-Wconversion;-Wsign-conversion;-Wmisleading-indentation;-Wduplicated-cond;-Wduplicated-branches;-Wlogical-op;-Wnull-dereference;-Wuseless-cast;-Wdouble-promotion;-Wformat=2;-Wlifetime"
)

# ################### Modules ####################
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(GNUInstallDirs)
include(CMakeBuildTypeUtils)

# ################### Packages ####################
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

# ################### ImFFmpeg #####################
add_executable(imffmpeg src/main.cpp imgui_backends/imgui_impl_glfw.cpp dependencies/ImGuiFileDialog/ImGuiFileDialog.cpp
                        imgui_backends/imgui_impl_opengl3.cpp)
target_include_directories(imffmpeg PUBLIC include imgui_backends dependencies)
target_link_libraries(imffmpeg PUBLIC CONAN_PKG::imgui CONAN_PKG::glfw CONAN_PKG::glew)
target_link_libraries(imffmpeg PUBLIC CONAN_PKG::boost CONAN_PKG::nlohmann_json)

