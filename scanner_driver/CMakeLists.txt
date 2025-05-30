# scanner_driver/CMakeLists.txt

# Create a library from the scanner driver source files
add_library(scanner_driver STATIC
    slice.cpp
    frame.cpp
    strip.cpp
)

# Optionally specify include directories (e.g., for header access)
target_include_directories(scanner_driver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
