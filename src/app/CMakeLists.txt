#Qt6 is already found at root level, just create the executable
qt_add_executable(korova WIN32 MACOSX_BUNDLE 
    main.cpp 
    mainwindow.cpp 
    mainwindow.h 
    mainwindow.ui 
    thumbnailwidget.cpp 
    thumbnailwidget.h 
    thumbnailcontainer.cpp 
    thumbnailcontainer.h 
    imageviewer.cpp 
    imageviewer.h
    scannerwaitdialog.cpp
    scannerwaitdialog.h
    calibrationwindow.cpp
    calibrationwindow.h
    icon.png
)

# Set application icon for all platforms
set(APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icon.png")
set(APP_ICON_RELATIVE "icon.png")

# Add icon as Qt resource for all platforms
qt_add_resources(korova "icons"
    PREFIX "/"
    FILES ${APP_ICON_RELATIVE}
)

# Platform-specific icon setup
if(WIN32)
    # Windows: Set executable icon (requires .ico file)
    message(STATUS "Windows icon added as resource. For .exe icon, create a .ico file manually.")
endif()

if(APPLE)
    # macOS: Set bundle properties
    set_target_properties(korova PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "Korova"
        MACOSX_BUNDLE_DISPLAY_NAME "Korova"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        MACOSX_BUNDLE_IDENTIFIER "com.soke.korova"
        MACOSX_BUNDLE_COPYRIGHT "Copyright © 2024 Soke Engineering"
    )
    
    # macOS: Create ICNS file using iconutil
    find_program(ICONUTIL_EXECUTABLE iconutil)
    if(ICONUTIL_EXECUTABLE)
        set(ICONSET_DIR "${CMAKE_CURRENT_BINARY_DIR}/icon.iconset")
        set(ICNS_FILE "${CMAKE_CURRENT_BINARY_DIR}/icon.icns")
        
        # Create iconset directory
        file(MAKE_DIRECTORY ${ICONSET_DIR})
        
        # Generate different icon sizes for macOS using sips
        find_program(SIPS_EXECUTABLE sips)
        if(SIPS_EXECUTABLE)
            add_custom_command(
                OUTPUT ${ICNS_FILE}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${ICONSET_DIR}
                COMMAND ${SIPS_EXECUTABLE} -z 16 16 ${APP_ICON} --out ${ICONSET_DIR}/icon_16x16.png
                COMMAND ${SIPS_EXECUTABLE} -z 32 32 ${APP_ICON} --out ${ICONSET_DIR}/icon_16x16@2x.png
                COMMAND ${SIPS_EXECUTABLE} -z 32 32 ${APP_ICON} --out ${ICONSET_DIR}/icon_32x32.png
                COMMAND ${SIPS_EXECUTABLE} -z 64 64 ${APP_ICON} --out ${ICONSET_DIR}/icon_32x32@2x.png
                COMMAND ${SIPS_EXECUTABLE} -z 128 128 ${APP_ICON} --out ${ICONSET_DIR}/icon_128x128.png
                COMMAND ${SIPS_EXECUTABLE} -z 256 256 ${APP_ICON} --out ${ICONSET_DIR}/icon_128x128@2x.png
                COMMAND ${SIPS_EXECUTABLE} -z 256 256 ${APP_ICON} --out ${ICONSET_DIR}/icon_256x256.png
                COMMAND ${SIPS_EXECUTABLE} -z 512 512 ${APP_ICON} --out ${ICONSET_DIR}/icon_256x256@2x.png
                COMMAND ${SIPS_EXECUTABLE} -z 512 512 ${APP_ICON} --out ${ICONSET_DIR}/icon_512x512.png
                COMMAND ${SIPS_EXECUTABLE} -z 1024 1024 ${APP_ICON} --out ${ICONSET_DIR}/icon_512x512@2x.png
                COMMAND ${ICONUTIL_EXECUTABLE} -c icns ${ICONSET_DIR} -o ${ICNS_FILE}
                DEPENDS ${APP_ICON}
                COMMENT "Creating macOS ICNS icon"
            )
            
            # Add the ICNS file as a source to ensure it's built before linking
            add_custom_target(icon_icns ALL DEPENDS ${ICNS_FILE})
            add_dependencies(korova icon_icns)
            
            # Set the bundle icon file property
            set_target_properties(korova PROPERTIES
                MACOSX_BUNDLE_ICON_FILE "icon.icns"
            )
            
            # Copy the ICNS file to the app bundle resources
            add_custom_command(TARGET korova POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_BUNDLE_DIR:korova>/Contents/Resources
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ICNS_FILE} $<TARGET_BUNDLE_DIR:korova>/Contents/Resources/icon.icns
            )
        else()
            message(WARNING "sips not found. macOS icon will not be set.")
        endif()
    else()
        message(WARNING "iconutil not found. macOS icon will not be set.")
    endif()
endif()

# Linux: Set desktop file icon
if(UNIX AND NOT APPLE)
    # Create desktop file for Linux
    configure_file(
        "${CMAKE_SOURCE_DIR}/korova.desktop.in"
        "${CMAKE_CURRENT_BINARY_DIR}/korova.desktop"
        @ONLY
    )
    
    # Install desktop file and icon
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/korova.desktop
        DESTINATION share/applications
    )
    install(FILES ${APP_ICON}
        DESTINATION share/pixmaps
        RENAME korova.png
    )
endif()

target_link_libraries(korova PRIVATE Qt::Core Qt::Widgets structures knokke ${OpenCV_LIBS})

#Include OpenCV headers
target_include_directories(korova PRIVATE ${OpenCV_INCLUDE_DIRS})