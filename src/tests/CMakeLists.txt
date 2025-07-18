cmake_minimum_required(VERSION 3.16)

# Enable testing
include(CTest)

# Find GTest if available
find_package(GTest QUIET)

set(TEST_SOURCES
    test_ftd2xxwrapper.cpp
    test_ftdienumerator.cpp
    test_opencv.cpp
    test_knokke_v1.cpp
    test_knokke_v1_integration.cpp
    test_knokke_v1_dual_interface.cpp
    test_knokke_v1_16bit.cpp
)

foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/src/drivers/scanners/knokke_v1
    )
    target_link_libraries(${test_name} PRIVATE knokke_v1 pthread dl)
    
    # Link OpenCV for the OpenCV test
    if(test_name STREQUAL "test_opencv")
        target_link_libraries(${test_name} PRIVATE ${OpenCV_LIBS})
        target_include_directories(${test_name} PRIVATE ${OpenCV_INCLUDE_DIRS})
    endif()
    
    if(GTest_FOUND)
        target_link_libraries(${test_name} PRIVATE GTest::gtest GTest::gtest_main)
    endif()
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach() 