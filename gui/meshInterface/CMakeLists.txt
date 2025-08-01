cmake_minimum_required(VERSION 3.16)
   project(meshInterface VERSION 0.1 LANGUAGES CXX)

   set(CMAKE_AUTOUIC ON)
   set(CMAKE_AUTOMOC ON)
   set(CMAKE_AUTORCC ON)
   set(CMAKE_CXX_STANDARD 17)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)
   set(CMAKE_INCLUDE_CURRENT_DIR ON)

   # Find Qt with Widgets and Sql components
   find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql)
   find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql)
   if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
       find_package(Qt6 REQUIRED COMPONENTS Core)
   endif()

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
   )

   if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
       qt_add_executable(meshInterface
           MANUAL_FINALIZATION
           ${PROJECT_SOURCES}
       )
   else()
       if(ANDROID)
           add_library(meshInterface SHARED
               ${PROJECT_SOURCES}
           )
       else()
           add_executable(meshInterface
               ${PROJECT_SOURCES}
           )
       endif()
   endif()

   # Link Widgets, Sql, and Core (for Qt6)
   target_link_libraries(meshInterface PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Sql)
   if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
       target_link_libraries(meshInterface PRIVATE Qt6::Core)
   endif()

   # macOS/iOS bundle settings
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

   include(GNUInstallDirs)
   install(TARGETS meshInterface
       BUNDLE DESTINATION .
   )
