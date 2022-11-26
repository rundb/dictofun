set(CMAKE_SYSTEM "Generic")
set(CMAKE_SYSTEM_PROCESSOR "arm")
set(CMAKE_CROSSCOMPILING "TRUE")

set(CMAKE_AR                    arm-none-eabi-ar)
set(CMAKE_ASM_COMPILER          arm-none-eabi-as)
set(CMAKE_C_COMPILER            arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER          arm-none-eabi-g++)
set(CMAKE_LINKER                arm-none-eabi-gcc)
set(CMAKE_OBJCOPY               arm-none-eabi-objcopy)

SET(CMAKE_EXE_LINKER_FLAGS      "-specs=nosys.specs")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM     NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY     ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE     ONLY)
