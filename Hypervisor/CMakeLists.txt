cmake_minimum_required(VERSION 3.20)
project(TaaprHypervisor C ASM_MASM)

# This CMakeLists.txt is for reference only
# Kernel drivers should be built using the Windows Driver Kit (WDK)
# Use the Visual Studio solution with WDK or the wdkbuild command

message(WARNING "
===============================================
IMPORTANT: Kernel drivers require WDK!
===============================================
This project cannot be built with standard CMake.
You need to:

1. Install Windows Driver Kit (WDK):
   https://docs.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk

2. Open Visual Studio with WDK workload

3. Create a new KMDF Driver project

4. Add these source files:
   - src/svm.c
   - src/svm_asm.asm
   - include/svm.h

5. Build in Test Mode with test signing enabled
===============================================
")

# Source files (for reference)
set(SOURCES
    src/svm.c
    src/svm_asm.asm
)

set(HEADERS
    include/svm.h
)

# Note: Actual kernel driver builds require WDK tools
# This CMake file is just for IDE integration and documentation
