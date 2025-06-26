# Qt6 is already found at root level, just create the executable
qt_add_executable(korova
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

target_link_libraries(korova
    PRIVATE
    Qt::Core
    Qt::Widgets
    structures
    knokke_v1
    ${OpenCV_LIBS}
)

# Include OpenCV headers
target_include_directories(korova PRIVATE ${OpenCV_INCLUDE_DIRS})