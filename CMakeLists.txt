cmake_minimum_required(VERSION 3.15)
project(OpenGLPBR CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(cpp)
include_directories(third_party)

file(GLOB_RECURSE SOURCES "cpp/*.cpp")
file(GLOB_RECURSE HEADERS "cpp/*.h")

add_executable(OpenGLPBR ${SOURCES} ${HEADERS})