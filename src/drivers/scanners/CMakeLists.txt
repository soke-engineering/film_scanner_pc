# Add all scanner implementations
add_library(knokke STATIC
    Knokke.cpp
    Knokke.h
)

# Cross-platform libusb-1.0 detection and linking
if(WIN32)
    # Windows: Use vcpkg or manual libusb installation
    find_library(LIBUSB_LIBRARY
        NAMES usb-1.0 libusb-1.0
        PATHS
            ${CMAKE_PREFIX_PATH}/lib
            ${CMAKE_PREFIX_PATH}/lib/x64
            ${CMAKE_PREFIX_PATH}/lib/x86
            "C:/vcpkg/installed/x64-windows/lib"
            "C:/vcpkg/installed/x86-windows/lib"
    )
    
    find_path(LIBUSB_INCLUDE_DIR
        NAMES libusb.h
        PATHS
            ${CMAKE_PREFIX_PATH}/include
            ${CMAKE_PREFIX_PATH}/include/libusb-1.0
            "C:/vcpkg/installed/x64-windows/include"
            "C:/vcpkg/installed/x86-windows/include"
    )
    
    if(LIBUSB_LIBRARY AND LIBUSB_INCLUDE_DIR)
        target_link_libraries(knokke ${LIBUSB_LIBRARY})
        target_include_directories(knokke PUBLIC ${LIBUSB_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "libusb-1.0 not found. Please install via vcpkg or manually.")
    endif()
    
elseif(APPLE)
    # macOS: Use Homebrew or pkg-config
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBUSB libusb-1.0)
        if(LIBUSB_FOUND)
            target_link_libraries(knokke ${LIBUSB_LIBRARIES})
            target_include_directories(knokke PUBLIC ${LIBUSB_INCLUDE_DIRS})
            target_compile_options(knokke PRIVATE ${LIBUSB_CFLAGS_OTHER})
        else()
            # Fallback to manual search
            find_library(LIBUSB_LIBRARY NAMES usb-1.0 PATHS /usr/local/lib /opt/homebrew/lib)
            find_path(LIBUSB_INCLUDE_DIR NAMES libusb.h PATHS /usr/local/include /opt/homebrew/include)
            if(LIBUSB_LIBRARY AND LIBUSB_INCLUDE_DIR)
                target_link_libraries(knokke ${LIBUSB_LIBRARY})
                target_include_directories(knokke PUBLIC ${LIBUSB_INCLUDE_DIR})
            else()
                message(FATAL_ERROR "libusb-1.0 not found. Please install via Homebrew: brew install libusb")
            endif()
        endif()
    else()
        message(FATAL_ERROR "pkg-config not found. Please install pkg-config")
    endif()
    
elseif(UNIX)
    # Linux: Use pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
    
    target_link_libraries(knokke ${LIBUSB_LIBRARIES})
    target_include_directories(knokke PUBLIC ${LIBUSB_INCLUDE_DIRS})
    target_compile_options(knokke PRIVATE ${LIBUSB_CFLAGS_OTHER})
    
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# Set C++ standard
target_compile_features(knokke PUBLIC cxx_std_17)

# Platform-specific compiler definitions
if(WIN32)
    target_compile_definitions(knokke PRIVATE WIN32_LEAN_AND_MEAN)
endif()