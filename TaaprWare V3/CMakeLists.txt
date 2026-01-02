cmake_minimum_required(VERSION 3.20)

project(TaaprWareV3)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============== TaaprWare DLL ==============
set(DLL_SOURCES
    dllmain.cpp
    luavm_load_hook.cpp
)

set(DLL_HEADERS
    roblox.h
    luavm_load_hook.h
    Scanner.hpp
    AntiCheck.hpp
)

# Enable MASM for x64 assembly hooks
enable_language(ASM_MASM)
set(ASM_SOURCES hooks.asm)

# Create Shared Library (DLL)
add_library(TaaprWareV3 SHARED ${DLL_SOURCES} ${DLL_HEADERS} ${ASM_SOURCES})

target_compile_definitions(TaaprWareV3 PRIVATE
    WIN32_LEAN_AND_MEAN
    _CRT_SECURE_NO_WARNINGS
)

target_link_libraries(TaaprWareV3 PRIVATE kernel32 user32 psapi)

set_target_properties(TaaprWareV3 PROPERTIES 
    SUFFIX ".dll"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ============== Manual Mapper Executable ==============
add_executable(ManualMapper ManualMapper.cpp)

target_compile_definitions(ManualMapper PRIVATE
    WIN32_LEAN_AND_MEAN
    _CRT_SECURE_NO_WARNINGS
)

target_link_libraries(ManualMapper PRIVATE kernel32 user32 ntdll)

set_target_properties(ManualMapper PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
