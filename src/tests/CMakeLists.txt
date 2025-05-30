cmake_minimum_required(VERSION 3.16)

# Enable testing
include(CTest)

# Find GTest if available
find_package(GTest QUIET)

set(TEST_SOURCES
    test_ftd2xxwrapper.cpp
    test_ftdienumerator.cpp
)

foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/src/drivers/scanners/knokke_v1
    )
    target_link_libraries(${test_name} PRIVATE knokke_v1 pthread dl)
    if(GTest_FOUND)
        target_link_libraries(${test_name} PRIVATE GTest::gtest GTest::gtest_main)
    endif()
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach() 