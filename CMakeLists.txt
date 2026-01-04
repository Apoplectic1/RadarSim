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
    add_definitions(-DQT_NO_DEBUG_OUTPUT)
    add_definitions(-DQT_NO_WARNING_OUTPUT)

    if(MSVC)
        add_compile_options(/O2)
    else()
        add_compile_options(-O2)
    endif()
else()
    add_definitions(-DQT_DEBUG)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt packages
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets)
else()
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets OpenGL)
endif()

# Common sources (shared utilities)
set(COMMON_SOURCES
    Common/Constants.h
    Common/GLUtils.h
)

# UI/MainWindow sources
set(UI_MAINWINDOW_SOURCES
    UI/MainWindow/RadarSim.cpp
    UI/MainWindow/RadarSim.h
    UI/MainWindow/RadarSim.ui
)

# UI/MainWindow/RadarPane sources (3D radar scene)
set(UI_RADARPANE_SOURCES
    UI/MainWindow/RadarPane/RadarGLWidget.cpp
    UI/MainWindow/RadarPane/RadarGLWidget.h
    UI/MainWindow/RadarPane/RadarSceneWidget.cpp
    UI/MainWindow/RadarPane/RadarSceneWidget.h
    UI/MainWindow/RadarPane/Sphere/SphereRenderer.cpp
    UI/MainWindow/RadarPane/Sphere/SphereRenderer.h
    UI/MainWindow/RadarPane/RadarSite/RadarSiteRenderer.cpp
    UI/MainWindow/RadarPane/RadarSite/RadarSiteRenderer.h
    UI/MainWindow/RadarPane/Cones/ReflectionRenderer.cpp
    UI/MainWindow/RadarPane/Cones/ReflectionRenderer.h
    UI/MainWindow/RadarPane/HeatMap/HeatMapRenderer.cpp
    UI/MainWindow/RadarPane/HeatMap/HeatMapRenderer.h
    UI/MainWindow/RadarPane/SlicingPlane/SlicingPlaneRenderer.cpp
    UI/MainWindow/RadarPane/SlicingPlane/SlicingPlaneRenderer.h
    UI/MainWindow/RadarPane/Debug/DebugRayRenderer.cpp
    UI/MainWindow/RadarPane/Debug/DebugRayRenderer.h
)

# UI/MainWindow/RCSPane sources (2D polar plot and RCS computation)
set(UI_RCSPANE_SOURCES
    UI/MainWindow/RCSPane/PolarPlot/PolarRCSPlot.cpp
    UI/MainWindow/RCSPane/PolarPlot/PolarRCSPlot.h
    UI/MainWindow/RCSPane/Compute/RCSCompute.cpp
    UI/MainWindow/RCSPane/Compute/RCSCompute.h
    UI/MainWindow/RCSPane/Compute/RCSTypes.h
    UI/MainWindow/RCSPane/Compute/BVHBuilder.cpp
    UI/MainWindow/RCSPane/Compute/BVHBuilder.h
    UI/MainWindow/RCSPane/Sampling/RCSSampler.h
    UI/MainWindow/RCSPane/Sampling/AzimuthCutSampler.cpp
    UI/MainWindow/RCSPane/Sampling/AzimuthCutSampler.h
    UI/MainWindow/RCSPane/Sampling/ElevationCutSampler.cpp
    UI/MainWindow/RCSPane/Sampling/ElevationCutSampler.h
)

# UI/MainWindow/PopOutPane sources
set(UI_POPOUTPANE_SOURCES
    UI/MainWindow/PopOutPane/PopOutWindow.cpp
    UI/MainWindow/PopOutPane/PopOutWindow.h
    UI/MainWindow/PopOutPane/FBORenderer.cpp
    UI/MainWindow/PopOutPane/FBORenderer.h
    UI/MainWindow/PopOutPane/TextureBlitWidget.cpp
    UI/MainWindow/PopOutPane/TextureBlitWidget.h
)

# UI/MainWindow/RadarPane/Camera sources
set(UI_CAMERA_SOURCES
    UI/MainWindow/RadarPane/Camera/CameraController.cpp
    UI/MainWindow/RadarPane/Camera/CameraController.h
)

# UI/ConfigurationWindow sources (includes config data)
set(UI_CONFIGWINDOW_SOURCES
    UI/ConfigurationWindow/ConfigurationWindow.cpp
    UI/ConfigurationWindow/ConfigurationWindow.h
    UI/ConfigurationWindow/AppSettings.cpp
    UI/ConfigurationWindow/AppSettings.h
    UI/ConfigurationWindow/BeamConfig.h
    UI/ConfigurationWindow/CameraConfig.h
    UI/ConfigurationWindow/SceneConfig.h
    UI/ConfigurationWindow/TargetConfig.h
)

# UI/MainWindow/Controls sources (self-contained control widgets)
set(UI_CONTROLS_SOURCES
    UI/MainWindow/Controls/RadarControlsWidget.cpp
    UI/MainWindow/Controls/RadarControlsWidget.h
    UI/MainWindow/Controls/TargetControlsWidget.cpp
    UI/MainWindow/Controls/TargetControlsWidget.h
    UI/MainWindow/Controls/RCSPlaneControlsWidget.cpp
    UI/MainWindow/Controls/RCSPlaneControlsWidget.h
)

# UI/ControlsWindow sources
set(UI_CONTROLSWINDOW_SOURCES
    UI/ControlsWindow/ControlsWindow.cpp
    UI/ControlsWindow/ControlsWindow.h
)

# Beam sources
set(BEAM_SOURCES
    Beam/RadarBeam.cpp
    Beam/RadarBeam.h
    Beam/BeamController.cpp
    Beam/BeamController.h
    Beam/ConicalBeam.cpp
    Beam/ConicalBeam.h
    Beam/PhasedArrayBeam.cpp
    Beam/PhasedArrayBeam.h
    Beam/SincBeam.cpp
    Beam/SincBeam.h
)

# Target sources
set(TARGET_SOURCES
    Target/WireframeTarget.cpp
    Target/WireframeTarget.h
    Target/WireframeTargetController.cpp
    Target/WireframeTargetController.h
    Target/WireframeShapes.h
    Target/Shapes/CubeWireframe.cpp
    Target/Shapes/CubeWireframe.h
    Target/Shapes/CylinderWireframe.cpp
    Target/Shapes/CylinderWireframe.h
    Target/Shapes/AircraftWireframe.cpp
    Target/Shapes/AircraftWireframe.h
    Target/Shapes/SphereWireframe.cpp
    Target/Shapes/SphereWireframe.h
    Target/Model/ModelManager.cpp
    Target/Model/ModelManager.h
)

# Create executable
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(RadarSim
        MANUAL_FINALIZATION
        main.cpp
        ${COMMON_SOURCES}
        ${UI_MAINWINDOW_SOURCES}
        ${UI_RADARPANE_SOURCES}
        ${UI_RCSPANE_SOURCES}
        ${UI_POPOUTPANE_SOURCES}
        ${UI_CAMERA_SOURCES}
        ${UI_CONTROLS_SOURCES}
        ${UI_CONFIGWINDOW_SOURCES}
        ${UI_CONTROLSWINDOW_SOURCES}
        ${BEAM_SOURCES}
        ${TARGET_SOURCES}
    )
else()
    add_executable(RadarSim
        main.cpp
        ${COMMON_SOURCES}
        ${UI_MAINWINDOW_SOURCES}
        ${UI_RADARPANE_SOURCES}
        ${UI_RCSPANE_SOURCES}
        ${UI_POPOUTPANE_SOURCES}
        ${UI_CAMERA_SOURCES}
        ${UI_CONTROLS_SOURCES}
        ${UI_CONFIGWINDOW_SOURCES}
        ${UI_CONTROLSWINDOW_SOURCES}
        ${BEAM_SOURCES}
        ${TARGET_SOURCES}
    )
endif()

# Include directories for proper header resolution
target_include_directories(RadarSim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Common
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/MainWindow
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/MainWindow/RadarPane
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/MainWindow/RadarPane/Sphere
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/MainWindow/RadarPane/RadarSite
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/MainWindow/RadarPane/Cones
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/MainWindow/RadarPane/HeatMap
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/MainWindow/RadarPane/SlicingPlane
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/MainWindow/RadarPane/Debug
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/MainWindow/RCSPane/PolarPlot
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/MainWindow/RCSPane/Compute
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/MainWindow/RCSPane/Sampling
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/MainWindow/PopOutPane
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/MainWindow/RadarPane/Camera
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/MainWindow/Controls
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/ConfigurationWindow
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/ControlsWindow
    ${CMAKE_CURRENT_SOURCE_DIR}/Beam
    ${CMAKE_CURRENT_SOURCE_DIR}/Target
    ${CMAKE_CURRENT_SOURCE_DIR}/Target/Shapes
    ${CMAKE_CURRENT_SOURCE_DIR}/Target/Model
)

# Link Qt libraries
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
