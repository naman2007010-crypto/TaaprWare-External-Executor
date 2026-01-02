cmake_minimum_required(VERSION 3.20)
project(TaaprWare-External CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/main.cpp
)

# Headers
set(HEADERS
    include/MemoryReader.hpp
    include/ScriptInjector.hpp
)

# Executable
add_executable(TaaprWare-External ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(TaaprWare-External PRIVATE include)

# Windows-specific settings
if(WIN32)
    target_compile_definitions(TaaprWare-External PRIVATE
        _CRT_SECURE_NO_WARNINGS
        UNICODE
        _UNICODE
    )
    
    # Subsystem: Console
    set_target_properties(TaaprWare-External PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
endif()

# Output directory
set_target_properties(TaaprWare-External PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

message(STATUS "TaaprWare External Executor configured")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
