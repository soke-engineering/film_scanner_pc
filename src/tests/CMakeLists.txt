cmake_minimum_required(VERSION 3.16)

#Enable testing
include(CTest)

#Find GTest if available
find_package(GTest QUIET)

set(TEST_SOURCES test_opencv.cpp)

foreach (test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_include_directories(${test_name} PRIVATE ${CMAKE_SOURCE_DIR}/src/drivers)
    target_link_libraries(${test_name} PRIVATE pthread dl)

    #Link OpenCV for the OpenCV test
    if (test_name STREQUAL "test_opencv")
        target_link_libraries(${test_name} PRIVATE ${OpenCV_LIBS})
        target_include_directories(${test_name} PRIVATE ${OpenCV_INCLUDE_DIRS})
    endif()

    if (GTest_FOUND)
        target_link_libraries(${test_name} PRIVATE GTest::gtest GTest::gtest_main)
    endif()
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()