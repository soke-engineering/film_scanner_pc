# Create the structures library
add_library(structures STATIC
    frame.cpp
    slice.cpp
    strip.cpp
)

# Make headers available
target_include_directories(structures PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)