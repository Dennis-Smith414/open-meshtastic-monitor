cmake_minimum_required(VERSION 3.16)
project(meshInterface VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Find Qt - try Qt6 first, fallback to Qt5
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Sql)
find_package(Qt6 REQUIRED COMPONENTS SerialPort)

set(PROJECT_SOURCES
    main.cpp
    loginWindow.cpp
    loginWindow.h
    loginWindow.ui
    mainapp.cpp
    mainapp.h
    MainApp.ui
    resources.qrc
    userdatabase.h
    userdatabase.cpp
    meshtastic_handler.h
    meshtastic_handler.cpp
)

# Create executable - works for both Qt5 and Qt6
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(meshInterface
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(meshInterface SHARED ${PROJECT_SOURCES})
    else()
        add_executable(meshInterface ${PROJECT_SOURCES})
    endif()
endif()

# Link libraries - same for both Qt5 and Qt6
target_link_libraries(meshInterface PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::SerialPort
)

# Bundle settings
if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.meshInterface)
endif()

set_target_properties(meshInterface PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_finalize_executable(meshInterface)
endif()

include(GNUInstallDirs)
install(TARGETS meshInterface
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
