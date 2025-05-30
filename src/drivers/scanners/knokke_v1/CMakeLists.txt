cmake_minimum_required(VERSION 3.16)
project(knokke_v1)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create the main library
add_library(knokke_v1 STATIC
    knokke_v1.cpp
    FTD2XXWrapper.cpp
    FTDIEnumerator.cpp
)

# Set include directories
target_include_directories(knokke_v1 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Platform-specific configuration for FTDI D2XX
if(WIN32)
    # Windows configuration
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        # 64-bit Windows
        set(FTDI_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/d2xx/windows/amd64)
        target_link_libraries(knokke_v1 PUBLIC ${FTDI_LIB_DIR}/ftd2xx.lib)
    else()
        # 32-bit Windows
        set(FTDI_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/d2xx/windows/i386)
        target_link_libraries(knokke_v1 PUBLIC ${FTDI_LIB_DIR}/ftd2xx.lib)
    endif()
    
    # Add Windows-specific include directory
    target_include_directories(knokke_v1 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/d2xx/windows)
    
elseif(APPLE)
    # macOS configuration
    set(FTDI_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/d2xx/macos/build)
    
    # Link against the static library
    target_link_libraries(knokke_v1 PUBLIC ${FTDI_LIB_DIR}/libftd2xx.a)
    
    # Add macOS-specific include directory
    target_include_directories(knokke_v1 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/d2xx/macos)
    
    # Suppress warnings from d2xx directory on macOS
    target_compile_options(knokke_v1 PRIVATE
        -isystem ${CMAKE_CURRENT_SOURCE_DIR}/d2xx/macos
        -Wno-gnu-anonymous-struct
        -Wno-nested-anon-types
    )
    
    # macOS-specific frameworks that might be needed
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(IOKIT_LIBRARY IOKit)
    target_link_libraries(knokke_v1 PUBLIC ${COREFOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
    
elseif(UNIX)
    # Linux configuration
    set(FTDI_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/d2xx/linux/linux-x86_64)
    
    # Static linking
    target_link_libraries(knokke_v1 PUBLIC ${FTDI_LIB_DIR}/libftd2xx-static.a)
    
    # Add Linux-specific include directory
    target_include_directories(knokke_v1 PUBLIC ${FTDI_LIB_DIR})
    
    # Suppress warnings from d2xx directory on Linux
    target_compile_options(knokke_v1 PRIVATE
        -isystem ${FTDI_LIB_DIR}
        -Wno-gnu-anonymous-struct
        -Wno-nested-anon-types
    )
    
    # Linux-specific libraries that might be needed
    target_link_libraries(knokke_v1 PUBLIC pthread dl)
    
endif()

# Link against structures library (assuming it exists)
target_link_libraries(knokke_v1 PUBLIC structures)

# Optional: Add compiler-specific flags
if(MSVC)
    target_compile_options(knokke_v1 PRIVATE /W4)
else()
    target_compile_options(knokke_v1 PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Optional: Set runtime library path for static linking (not needed but kept for reference)
# if(UNIX AND NOT APPLE)
#     set_target_properties(knokke_v1 PROPERTIES
#         INSTALL_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/d2xx/linux/linux-x86_64"
#         BUILD_WITH_INSTALL_RPATH TRUE
#     )
# elseif(APPLE)
#     set_target_properties(knokke_v1 PROPERTIES
#         INSTALL_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/d2xx/macos/build"
#         BUILD_WITH_INSTALL_RPATH TRUE
#     )
# endif()