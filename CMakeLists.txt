cmake_minimum_required(VERSION 3.15)

if (POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif (POLICY CMP0091)

project(winfetch VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

if (MSVC)
    add_definitions(-DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS=1)
    add_compile_options("/utf-8")
endif ()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(winfetch
    src/winfetch.cpp
)

set_property(TARGET winfetch PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(winfetch PRIVATE
    Version
    DXGI
    UxTheme
)

add_executable(cpu example/cpu.cpp)

