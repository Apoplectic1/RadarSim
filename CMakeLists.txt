cmake_minimum_required(VERSION 3.14)

project(RadarSim LANGUAGES CXX)

set(PROJECT_VERSION "1.0.0")
set(PROJECT_VERSION_MAJOR "1")
set(PROJECT_VERSION_MINOR "0")

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Configure debug/release options
if(CMAKE_BUILD_TYPE MATCHES "Release")
    # Disable debug output in release mode
    add_definitions(-DQT_NO_DEBUG_OUTPUT)
    add_definitions(-DQT_NO_WARNING_OUTPUT)  # Optional: also disable warnings
    
    # Enable optimizations
    if(MSVC)
        add_compile_options(/O2)  # MSVC optimization
    else()
        add_compile_options(-O2)  # GCC/Clang optimization
    endif()
else()
    # Debug-specific options
    add_definitions(-DQT_DEBUG)
endif()

# Output the build type for verification
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Debug output enabled: ${CMAKE_BUILD_TYPE} MATCHES Debug")



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

# Component sources
set(COMPONENT_SOURCES
    Component/Component.cpp
    Component/Component.h
    Component/Entity.cpp
    Component/Entity.h
)

# ModelManager sources
set(MODEL_MANAGER_SOURCES
    ModelManager/ModelManager.cpp
    ModelManager/ModelManager.h
)

# RadarBeam classes
set(RADAR_BEAM_SOURCES
    RadarBeam/BeamController.cpp
    RadarBeam/BeamController.h
    RadarBeam/ConicalBeam.cpp
    RadarBeam/ConicalBeam.h
    RadarBeam/EllipticalBeam.cpp
    RadarBeam/EllipticalBeam.h
    RadarBeam/PhasedArrayBeam.cpp
    RadarBeam/PhasedArrayBeam.h
    RadarBeam/RadarBeam.cpp
    RadarBeam/RadarBeam.h
)

# RadarSceneWidget and related component classes
set(RADAR_SCENE_WIDGET_SOURCES
    RadarSceneWidget/CameraController.cpp
    RadarSceneWidget/CameraController.h
    RadarSceneWidget/RadarGLWidget.cpp
    RadarSceneWidget/RadarGLWidget.h
    RadarSceneWidget/RadarSceneWidget.cpp
    RadarSceneWidget/RadarSceneWidget.h
    RadarSceneWidget/SphereRenderer.cpp
    RadarSceneWidget/SphereRenderer.h
    RadarSceneWidget/SphereWidget.cpp
    RadarSceneWidget/SphereWidget.h
)

# Main application sources
set(PROJECT_SOURCES
    main.cpp
    RadarSim.cpp
    RadarSim.h
    RadarSim.ui
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
