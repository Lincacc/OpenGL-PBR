cmake_minimum_required(VERSION 3.15)
project(OpenGLPBR LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakePredefinedTargets")

# glfw sub project
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/glfw)

# glm sub project
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/glm)

# glad sub project
add_library(glad STATIC third_party/opengl/glad.c)
target_include_directories(glad PUBLIC third_party/opengl)

# imgui sub project
add_library(imgui STATIC
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC third_party/imgui third_party/imgui/backends)
target_link_libraries(imgui PUBLIC glfw glad)

set_target_properties(glfw glad imgui update_mappings PROPERTIES FOLDER "ThirdParty")

include_directories(cpp)

file(GLOB_RECURSE SOURCES "cpp/*.cpp")
file(GLOB_RECURSE HEADERS "cpp/*.h")

add_executable(OpenGLPBR WIN32 ${SOURCES} ${HEADERS})

if(MSVC)
    target_link_options(OpenGLPBR PRIVATE "/ENTRY:mainCRTStartup")
    target_compile_options(OpenGLPBR PRIVATE /utf-8)
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT OpenGLPBR)

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${SOURCES} ${HEADERS})

target_link_libraries(OpenGLPBR PRIVATE glfw glm::glm imgui glad legacy_stdio_definitions pdh comdlg32)

# Set the working directory for the debugger to the source directory (CMAKE_SOURCE_DIR = E:\repo\OpenGL-PBR)
set_target_properties(OpenGLPBR PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")