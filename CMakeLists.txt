cmake_minimum_required(VERSION 3.14)

project(RadarSim LANGUAGES CXX)

set(PROJECT_VERSION "1.0.0")
set(PROJECT_VERSION_MAJOR "1")
set(PROJECT_VERSION_MINOR "0")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt packages with the correct modules
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets)
else()
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets OpenGL)
endif()
include_directories(${Qt${QT_VERSION_MAJOR}_INCLUDE_DIRS})

# Main application sources
set(PROJECT_SOURCES
    main.cpp
    RadarSim.cpp
    RadarSim.h
    RadarSim.ui
)

# RadarBeam classes
set(RADAR_BEAM_SOURCES
    RadarBeam/RadarBeam.cpp
    RadarBeam/RadarBeam.h
    RadarBeam/ConicalBeam.cpp
    RadarBeam/ConicalBeam.h
    RadarBeam/EllipticalBeam.cpp
    RadarBeam/EllipticalBeam.h
    RadarBeam/PhasedArrayBeam.cpp
    RadarBeam/PhasedArrayBeam.h
    RadarBeam/BeamController.cpp
    RadarBeam/BeamController.h
)

# RadarSceneWidget and related component classes
set(RADAR_SCENE_WIDGET_SOURCES
    RadarSceneWidget/RadarSceneWidget.cpp
    RadarSceneWidget/RadarSceneWidget.h
    RadarSceneWidget/SphereRenderer.cpp
    RadarSceneWidget/SphereRenderer.h
    RadarSceneWidget/BeamController.cpp
    RadarSceneWidget/BeamController.h
    RadarSceneWidget/CameraController.cpp
    RadarSceneWidget/CameraController.h
    RadarSceneWidget/SphereWidget.cpp
    RadarSceneWidget/SphereWidget.h
    RadarSceneWidget/CameraController.cpp
    RadarSceneWidget/CameraController.h
)
# Component sources
set(COMPONENT_SOURCES
    Entity.cpp
    Entity.h
    Component.cpp
    Component.h
)

# ModelManager sources
set(MODEL_MANAGER_SOURCES
    ModelManager.cpp
    ModelManager.h
)

# Create executable
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(RadarSim
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${RADAR_BEAM_SOURCES}
        ${RADAR_SCENE_WIDGET_SOURCES}
        ${MODEL_MANAGER_SOURCES}
        ${COMPONENT_SOURCES}
    )
else()
    add_executable(RadarSim
        ${PROJECT_SOURCES}
        ${RADAR_BEAM_SOURCES}
        ${RADAR_SCENE_WIDGET_SOURCES}
        ${MODEL_MANAGER_SOURCES}
        ${COMPONENT_SOURCES}
    )
endif()

# Define target properties for Android with Qt 6 as:
# set_property(TARGET RadarSim APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#     ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

# Include directories
target_include_directories(RadarSim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/RadarBeam
    ${CMAKE_CURRENT_SOURCE_DIR}/RadarSceneWidget
)

# Link libraries
target_link_libraries(RadarSim PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::OpenGL
)

# Link libraries with the correct modules
target_link_libraries(RadarSim PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)

# Add OpenGL libraries based on Qt version
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    target_link_libraries(RadarSim PRIVATE Qt${QT_VERSION_MAJOR}::OpenGLWidgets)
else()
    target_link_libraries(RadarSim PRIVATE Qt${QT_VERSION_MAJOR}::OpenGL)
endif()

# Add system-specific OpenGL library
if(WIN32)
    target_link_libraries(RadarSim PRIVATE opengl32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(RadarSim PRIVATE GL)
endif()

set_target_properties(RadarSim PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(RadarSim)
endif()