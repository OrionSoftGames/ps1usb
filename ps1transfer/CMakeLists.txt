cmake_minimum_required(VERSION 3.5)

if(WIN32)
    # Force building x86 on Windows since the FTDI libs are x86
    set(CMAKE_GENERATOR_PLATFORM Win32 CACHE INTERNAL "")
endif()

project(ps1transfer C)

if(UNIX)
    # Linux users must install libftdi1 (Under Debian: sudo apt-get install libftdi1)
    find_package(LibFTDI1 REQUIRED)
endif()

add_executable(${PROJECT_NAME}
    main.c
    lodepng.c
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ftd2xx.lib)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -D_CRT_SECURE_NO_WARNINGS)
elseif(UNIX)
    target_include_directories(${PROJECT_NAME} PRIVATE /usr/include/libftdi1/)
    target_link_libraries(${PROJECT_NAME} PRIVATE ftdi1)
endif()
