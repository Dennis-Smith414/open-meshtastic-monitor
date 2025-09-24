cmake_minimum_required(VERSION 3.16)
project(meshInterface VERSION 0.1 LANGUAGES CXX)

# Force Qt6 and set correct path for Arch Linux
set(CMAKE_PREFIX_PATH "/usr/lib/qt6")

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Explicitly look for Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Sql SerialPort WebEngineWidgets)

# Modern protobuf finding
find_package(Protobuf REQUIRED)

# Find Abseil with all required components
find_package(absl REQUIRED COMPONENTS
    log_internal_check_op
    log_internal_message
    strings
    base
    log
    log_internal_format
    log_internal_globals
    log_internal_log_impl
    log_internal_proto
    log_internal_check_op
    log_internal_conditions
    log_internal_voidify
)

# Include directories
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/meshtastic)

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
    #---protobuf scheamas
    meshtastic/mesh.pb.cc       
    meshtastic/telemetry.pb.cc
    meshtastic/portnums.pb.cc
    meshtastic/config.pb.cc
    meshtastic/channel.pb.cc
    meshtastic/device_ui.pb.cc
    meshtastic/module_config.pb.cc
    meshtastic/xmodem.pb.cc
    meshtastic/mesh.pb.h         
    meshtastic/telemetry.pb.h
    meshtastic/portnums.pb.h
    meshtastic/config.pb.h
    meshtastic/channel.pb.h
    meshtastic/device_ui.pb.h
    meshtastic/module_config.pb.h
    meshtastic/xmodem.pb.h
)

# Create executable
add_executable(meshInterface
    ${PROJECT_SOURCES}
    resources.qrc
)

# Enable Qt6 automoc, autorcc, autouic for the target
target_link_libraries(meshInterface PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    Qt6::SerialPort
    protobuf::libprotobuf
    absl::log_internal_check_op
    absl::log_internal_message
    absl::strings
    absl::base
    absl::log
    absl::log_internal_format
    absl::log_internal_globals
    Qt6::WebEngineWidgets
)

# Set target properties for Qt6
set_target_properties(meshInterface PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS meshInterface
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
