cmake_minimum_required(VERSION 3.16)
project(VolumeManager VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tell CMake to process Qt's special macros
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt 6 Components
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# 1. Define ONE unified list of source files (including the UI file)
set(SOURCES
    main.cpp
    mainwindow.h
    mainwindow.cpp
    mainwindow.ui
    audiobackend.h
    audiobackend.cpp
)

# 2. Append the Resource file (.rc) instead of the icon (.ico) ONLY on Windows
if(WIN32)
    set(SOURCES ${SOURCES} app.rc)
endif()

# 3. Create a single executable with the WIN32 flag using Qt's recommended wrapper
qt_add_executable(VolumeManager WIN32 ${SOURCES})

# 4. Link ALL required libraries (Qt + Windows Audio APIs) to your target
target_link_libraries(VolumeManager PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ole32        # Windows COM Initialization
    mmdevapi     # Core Audio (WASAPI) APIs
)