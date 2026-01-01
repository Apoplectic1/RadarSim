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

# ModelManager sources
set(MODEL_MANAGER_SOURCES
    ModelManager/ModelManager.cpp
    ModelManager/ModelManager.h
)

# RCSCompute sources (GPU ray tracing for RCS calculations)
set(RCS_COMPUTE_SOURCES
    RCSCompute/RCSTypes.h
    RCSCompute/BVHBuilder.cpp
    RCSCompute/BVHBuilder.h
    RCSCompute/RCSCompute.cpp
    RCSCompute/RCSCompute.h
    RCSCompute/RCSSampler.h
    RCSCompute/AzimuthCutSampler.cpp
    RCSCompute/AzimuthCutSampler.h
    RCSCompute/ElevationCutSampler.cpp
    RCSCompute/ElevationCutSampler.h
    RCSCompute/SlicingPlaneRenderer.cpp
    RCSCompute/SlicingPlaneRenderer.h
)

# ReflectionRenderer sources (RCS lobe visualization)
set(REFLECTION_RENDERER_SOURCES
    ReflectionRenderer/ReflectionRenderer.cpp
    ReflectionRenderer/ReflectionRenderer.h
    ReflectionRenderer/HeatMapRenderer.cpp
    ReflectionRenderer/HeatMapRenderer.h
)

# WireframeTarget sources
set(WIREFRAME_TARGET_SOURCES
    WireframeTarget/WireframeShapes.h
    WireframeTarget/WireframeTarget.cpp
    WireframeTarget/WireframeTarget.h
    WireframeTarget/WireframeTargetController.cpp
    WireframeTarget/WireframeTargetController.h
    WireframeTarget/CubeWireframe.cpp
    WireframeTarget/CubeWireframe.h
    WireframeTarget/CylinderWireframe.cpp
    WireframeTarget/CylinderWireframe.h
    WireframeTarget/AircraftWireframe.cpp
    WireframeTarget/AircraftWireframe.h
    WireframeTarget/SphereWireframe.cpp
    WireframeTarget/SphereWireframe.h
)

# RadarBeam classes - CRITICAL: Use absolute paths from source directory
set(RADAR_BEAM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/RadarBeam/BeamController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RadarBeam/BeamController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RadarBeam/ConicalBeam.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RadarBeam/ConicalBeam.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RadarBeam/PhasedArrayBeam.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RadarBeam/PhasedArrayBeam.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RadarBeam/SincBeam.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RadarBeam/SincBeam.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RadarBeam/RadarBeam.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RadarBeam/RadarBeam.h
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
    RadarSceneWidget/FBORenderer.cpp
    RadarSceneWidget/FBORenderer.h
    RadarSceneWidget/TextureBlitWidget.cpp
    RadarSceneWidget/TextureBlitWidget.h
)

# Config sources (settings and configuration)
set(CONFIG_SOURCES
    Config/AppSettings.cpp
    Config/AppSettings.h
    Config/BeamConfig.h
    Config/CameraConfig.h
    Config/TargetConfig.h
    Config/SceneConfig.h
)

# PolarPlot sources (2D polar RCS visualization)
set(POLAR_PLOT_SOURCES
    PolarPlot/PolarRCSPlot.cpp
    PolarPlot/PolarRCSPlot.h
)

# Main application sources
set(PROJECT_SOURCES
    main.cpp
    RadarSim.cpp
    RadarSim.h
    RadarSim.ui
    GLUtils.h
    Constants.h
    PopOutWindow.cpp
    PopOutWindow.h
    ConfigurationWindow.cpp
    ConfigurationWindow.h
)

# Debug: Print source files
message(STATUS "RadarBeam sources:")
foreach(src ${RADAR_BEAM_SOURCES})
    message(STATUS "  ${src}")
endforeach()

# Create executable
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(RadarSim
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${RADAR_BEAM_SOURCES}
        ${RADAR_SCENE_WIDGET_SOURCES}
        ${MODEL_MANAGER_SOURCES}
        ${WIREFRAME_TARGET_SOURCES}
        ${RCS_COMPUTE_SOURCES}
        ${REFLECTION_RENDERER_SOURCES}
        ${CONFIG_SOURCES}
        ${POLAR_PLOT_SOURCES}
    )
else()
    add_executable(RadarSim
        ${PROJECT_SOURCES}
        ${RADAR_BEAM_SOURCES}
        ${RADAR_SCENE_WIDGET_SOURCES}
        ${MODEL_MANAGER_SOURCES}
        ${WIREFRAME_TARGET_SOURCES}
        ${RCS_COMPUTE_SOURCES}
        ${REFLECTION_RENDERER_SOURCES}
        ${CONFIG_SOURCES}
        ${POLAR_PLOT_SOURCES}
    )
endif()

# CRITICAL: Include directories for proper header resolution
target_include_directories(RadarSim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/RadarBeam
    ${CMAKE_CURRENT_SOURCE_DIR}/RadarSceneWidget
    ${CMAKE_CURRENT_SOURCE_DIR}/ModelManager
    ${CMAKE_CURRENT_SOURCE_DIR}/WireframeTarget
    ${CMAKE_CURRENT_SOURCE_DIR}/RCSCompute
    ${CMAKE_CURRENT_SOURCE_DIR}/ReflectionRenderer
    ${CMAKE_CURRENT_SOURCE_DIR}/Config
    ${CMAKE_CURRENT_SOURCE_DIR}/PolarPlot
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