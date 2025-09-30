cmake_minimum_required(VERSION 3.19)
project(korova LANGUAGES CXX)

#Generate compile_commands.json for better IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#OpenCV Configuration
#Set minimum required OpenCV version
set(REQUIRED_OPENCV_VERSION "4.5.0")

#Configure OpenCV for static linking
set(OpenCV_STATIC ON)
set(OpenCV_USE_STATIC_LIBS ON)

#Find OpenCV
find_package(OpenCV REQUIRED)

#Check OpenCV version
if (OpenCV_VERSION VERSION_LESS REQUIRED_OPENCV_VERSION)
    message(FATAL_ERROR
        "OpenCV version ${OpenCV_VERSION} is too old. "
        "Required version: ${REQUIRED_OPENCV_VERSION} or higher. "
        "Please run 'invoke bootstrap' to install the correct version.")
endif()

message(STATUS "Found OpenCV: ${OpenCV_VERSION}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")

#Find Qt6 at the root level
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)
qt_standard_project_setup()

#Add the src directory which contains everything
add_subdirectory(src)

#Install and deployment
include(GNUInstallDirs)
install(TARGETS korova
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET korova
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})