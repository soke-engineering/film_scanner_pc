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
)

target_link_libraries(korova PRIVATE Qt::Core Qt::Widgets structures ${OpenCV_LIBS})

#Include OpenCV headers
target_include_directories(korova PRIVATE ${OpenCV_INCLUDE_DIRS})