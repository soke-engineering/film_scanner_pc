cmake_minimum_required(VERSION 3.16)

#Create the knokke_v1 example executable
    add_executable(knokke_v1_example knokke_v1_example.cpp)

#Set include directories
        target_include_directories(knokke_v1_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} / drivers /
                                   scanners / knokke_v1)

#Link against the knokke_v1 library and other dependencies
            target_link_libraries(knokke_v1_example PRIVATE knokke_v1 pthread dl)

#Set compiler flags
                if (MSVC) target_compile_options(knokke_v1_example PRIVATE / W4) else()
                    target_compile_options(knokke_v1_example PRIVATE - Wall - Wextra - Wpedantic)
                        endif()