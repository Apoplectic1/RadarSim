cmake_minimum_required(VERSION 3.16)
project(RadarSim VERSION 0.1 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt specific CMake functionality
set(CMAKE_AUTOMOC ON)  # Meta-Object Compiler
set(CMAKE_AUTORCC ON)  # Resource Compiler
set(CMAKE_AUTOUIC ON)  # UI Compiler

# Find Qt packages
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)

# List source files
set(SOURCES
    main.cpp
    RadarSim.cpp
    SphereWidget.cpp
)

# List header files
set(HEADERS
    RadarSim.h
    SphereWidget.h
)

# List UI files
set(UI_FILES
    RadarSim.ui
)

# Define the executable
add_executable(RadarSim
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
)

# Link Qt libraries
target_link_libraries(RadarSim PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
)

# Windows-specific configurations
if(WIN32)
    set_target_properties(RadarSim PROPERTIES
        WIN32_EXECUTABLE TRUE  # Create a GUI application
    )
endif()
