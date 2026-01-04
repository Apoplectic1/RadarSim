# CLAUDE.md - RadarSim Project Guide

## Project Overview

RadarSim is a Qt-based 3D radar simulation application written in C++17 with OpenGL 4.3 core rendering. It visualizes radar beams with interactive controls, featuring a floating Configuration window and a component-based architecture supporting multiple beam types. Includes GPU-accelerated ray tracing for radar cross-section (RCS) calculations.

- **Qt Version**: 6.10.1 (with Qt5 fallback support)
- **OpenGL**: 4.3 Core Profile (compute shaders for RCS ray tracing)
- **Build System**: CMake 3.14+
- **Platform**: Cross-platform (Windows, Linux, macOS)

## Build Commands

```bash
# Configure (from project root)
cmake --preset Qt-Debug      # Debug build
cmake --preset Qt-Release    # Release build

# Build
cmake --build out/build/debug
cmake --build out/build/release
```

## Project Structure

```
RadarSim/
├── main.cpp                    # Entry point, OpenGL context setup
├── RadarSim.h/.cpp             # Main window with menu bar, manages floating windows
├── ConfigurationWindow.h/.cpp  # Floating configuration window (display/beam/target options)
├── ControlsWindow.h/.cpp       # Floating controls window (radar/target/RCS plane parameters)
├── PopOutWindow.h/.cpp         # Pop-out window container for detached views
├── Constants.h                 # Compile-time constants (see Constants section below)
├── GLUtils.h                   # OpenGL error checking utilities
├── Controls/                   # Self-contained control panel widgets
│   ├── RadarControlsWidget.h/.cpp    # Radar controls (radius, theta, phi)
│   ├── TargetControlsWidget.h/.cpp   # Target controls (position, rotation, scale)
│   └── RCSPlaneControlsWidget.h/.cpp # RCS plane controls (cut type, offset, thickness)
├── Config/                     # Settings and configuration system
│   ├── AppSettings.h/.cpp      # Main settings manager with profile support
│   ├── BeamConfig.h            # Beam parameters (type, width, color, opacity)
│   ├── CameraConfig.h          # Camera state (distance, angles, focus)
│   ├── TargetConfig.h          # Target parameters (position, rotation, scale)
│   └── SceneConfig.h           # Scene options (radius, radar position, visibility)
├── RadarBeam/                  # Beam visualization module
│   ├── RadarBeam.h/.cpp        # Base beam class (VAO/VBO/shaders)
│   ├── ConicalBeam.h/.cpp      # Simple cone geometry (uniform intensity)
│   ├── PhasedArrayBeam.h/.cpp  # Array beam with side lobes
│   ├── SincBeam.h/.cpp         # Sinc² pattern with intensity falloff
│   └── BeamController.h/.cpp   # Component wrapper for beams
├── RadarSceneWidget/           # OpenGL scene management
│   ├── RadarGLWidget.h/.cpp    # Modern QOpenGLWidget with component architecture
│   ├── SphereRenderer.h/.cpp   # Sphere/grid/axes component
│   ├── RadarSiteRenderer.h/.cpp # Radar site dot rendering
│   ├── RadarSceneWidget.h/.cpp # Scene container
│   ├── CameraController.h/.cpp # View/camera controls
│   ├── FBORenderer.h/.cpp      # Framebuffer object for pop-out windows
│   └── TextureBlitWidget.h/.cpp # Displays FBO texture in pop-out window
├── ModelManager/               # 3D model handling
│   └── ModelManager.h/.cpp     # Model loading component
├── WireframeTarget/            # Solid target visualization (for RCS calculations)
│   ├── WireframeShapes.h       # WireframeType enum
│   ├── WireframeTarget.h/.cpp  # Base target class (GL_TRIANGLES solid rendering)
│   ├── WireframeTargetController.h/.cpp  # Component wrapper
│   ├── CubeWireframe.h/.cpp    # Solid cube (6 faces, 12 triangles)
│   ├── CylinderWireframe.h/.cpp# Solid cylinder (caps + sides)
│   ├── AircraftWireframe.h/.cpp# Solid fighter jet model
│   └── SphereWireframe.h/.cpp  # Geodesic sphere (icosahedron subdivision)
├── ReflectionRenderer/         # RCS reflection visualization
│   ├── ReflectionRenderer.h/.cpp  # Colored cone visualization of reflected energy
│   └── HeatMapRenderer.h/.cpp     # Smooth gradient heat map on radar sphere
├── RCSCompute/                 # GPU ray tracing for RCS calculations
│   ├── RCSTypes.h              # GPU-aligned structs (Ray, BVHNode, Triangle, HitResult)
│   ├── BVHBuilder.h/.cpp       # SAH-based Bounding Volume Hierarchy construction
│   ├── RCSCompute.h/.cpp       # OpenGL 4.3 compute shader ray tracer
│   ├── RCSSampler.h            # Abstract interface for RCS sampling
│   ├── AzimuthCutSampler.h/.cpp    # Horizontal plane cut sampling
│   ├── ElevationCutSampler.h/.cpp  # Vertical plane cut sampling
│   └── SlicingPlaneRenderer.h/.cpp # Visualize sampling plane in 3D
├── PolarPlot/                  # 2D polar RCS visualization
│   └── PolarRCSPlot.h/.cpp     # OpenGL widget for polar RCS plot
└── Docs/                       # Detailed documentation
    ├── architecture.md         # Threading model, GPU pipeline, component patterns
    ├── optimization_patterns.md # Future async/threading optimizations
    ├── adding_features.md      # How to add beams, targets, controls, components
    ├── ui_patterns.md          # Slider sync, window layouts, Qt patterns
    └── shadow_system.md        # GPU shadow implementation details
```

## Architecture

The project uses a component-based architecture. See `Docs/architecture.md` for full details.

**Components** (owned by `RadarGLWidget`):
- **SphereRenderer**: Sphere, grid lines, and axes
- **RadarSiteRenderer**: Radar site position dot
- **BeamController**: Radar beam rendering (Conical, Sinc, Phased)
- **CameraController**: View transformations, mouse interaction, inertia
- **WireframeTargetController**: Solid target shapes for RCS
- **RCSCompute**: GPU ray tracing for radar cross-section
- **ReflectionRenderer/HeatMapRenderer**: RCS visualization

**Event-Driven Rendering**: No continuous render loop. Updates triggered by mouse/slider events. RCS computation runs synchronously in `paintGL()`.

## Coding Conventions

### Naming
- Member variables: trailing underscore (`radius_`, `showBeam_`)
- Slots: `on*` prefix (`onRadiusSliderValueChanged`)
- Qt signals: descriptive action names (`beamUpdated`, `positionChanged`)

### Qt Patterns
- Use `blockSignals()` to prevent circular updates between sliders/spinboxes
- Parent-child hierarchy for automatic memory cleanup
- `Q_OBJECT` macro required for signal/slot support

### OpenGL Patterns
- Use `QOpenGLFunctions_4_5_Core` for type-safe GL calls
- VAO/VBO/EBO pattern for geometry
- Shader sources stored as `std::string_view` members

### Critical OpenGL Rules

**Dynamic Geometry Buffers**: Use raw `GLuint` buffer IDs instead of `QOpenGLBuffer` for geometry that changes at runtime. `QOpenGLBuffer::allocate()` can only be called once.

**Context Requirements**: Never call OpenGL functions from constructors - GL context isn't ready. Use `geometryDirty_` flag to defer GPU uploads.

**Deferred Beam Type Changes**: Beam type changes must be deferred to `paintGL()` where GL context exists. Deleting a beam outside GL context leaks resources.

**Signal Connections for Animation**: Components with timers (e.g., inertia) must connect to `update()`:
```cpp
connect(cameraController_, &CameraController::viewChanged,
        this, QOverload<>::of(&QWidget::update));
```

## Key Files for Common Tasks

| Task | Files |
|------|-------|
| Add new beam type | `RadarBeam/RadarBeam.h` (enum), new class, factory in `createBeam()` |
| Add new solid target | `WireframeShapes.h` (enum), new class, factory in `createTarget()` |
| Modify radar controls | `Controls/RadarControlsWidget.cpp`, `RadarSim.cpp` |
| Modify target controls | `Controls/TargetControlsWidget.cpp`, `RadarSim.cpp` |
| Modify RCS plane controls | `Controls/RCSPlaneControlsWidget.cpp`, `RadarSim.cpp` |
| Configuration options | `ConfigurationWindow.cpp`, `RadarSim.cpp` |
| Change rendering | `RadarGLWidget.cpp` (`paintGL()`), component `render()` methods |
| Adjust camera | `CameraController.cpp` |
| RCS ray tracing | `RCSCompute/RCSCompute.cpp`, `BVHBuilder.cpp` |
| RCS sampling/polar plot | `AzimuthCutSampler.cpp`, `ElevationCutSampler.cpp`, `PolarRCSPlot.cpp` |
| Reflection lobes | `ReflectionRenderer/ReflectionRenderer.cpp` |
| RCS heat map | `ReflectionRenderer/HeatMapRenderer.cpp` |
| Pop-out windows | `PopOutWindow.cpp`, `FBORenderer.cpp`, `TextureBlitWidget.cpp` |
| Add compile-time constant | `Constants.h` |
| Add saved setting | `Config/*Config.h`, `AppSettings.cpp`, `RadarSim.cpp` |

See `Docs/adding_features.md` for detailed how-to guides with code examples.

## Settings/Configuration System

- **Session Persistence**: State auto-saved on exit, restored on next launch
- **Named Profiles**: Save/load multiple named configurations
- **File Storage**: `%APPDATA%/RadarSim/` (Windows) or `~/.config/RadarSim/` (Linux/macOS)
- **Namespace**: Config classes use `RSConfig` namespace to avoid conflict with `RadarSim` class

## RCSCompute (GPU Ray Tracing)

GPU-accelerated ray tracing for RCS via beam-target intersection. Uses OpenGL 4.3 compute shaders.

**GPU-aligned Data Structures:**
```cpp
struct Ray       { vec4 origin, direction; };                    // 32 bytes
struct BVHNode   { vec4 boundsMin, boundsMax; };                  // 32 bytes
struct Triangle  { vec4 v0, v1, v2; };                            // 48 bytes
struct HitResult { vec4 hitPoint, normal, reflection; uint ids; };// 64 bytes
```

**BVH Construction**: Surface Area Heuristic (SAH), 12-bin binning, max leaf size 4 triangles.

See `Docs/architecture.md` for threading model and GPU pipeline details.

## Shadow System

GPU ray-traced shadows from RCSCompute. Shadow map texture (64x157) stores hit distances. Beam fragment shader samples and discards fragments behind hit points.

See `Docs/shadow_system.md` for implementation details and GLSL code.

## UI Layout

- **Controls Window** (LEFT): Radar, target, RCS plane sliders. Fixed width 270px.
- **Configuration Window** (RIGHT): Display/beam/viz toggles.
- **0.5 Increment Sliders**: Slider range is 2x actual. Use `blockSignals()` for sync.
- **Double-click reset**: Event filter in `RadarSim::eventFilter()` resets to `Constants::Defaults`.

See `Docs/ui_patterns.md` for full code examples and window layouts.

## Mouse Controls

| Button | Action |
|--------|--------|
| **Left Mouse** | Rotate/orbit the scene |
| **Middle Mouse** | Pan the scene |
| **Scroll Wheel** | Zoom in/out |
| **Left Double-Click** | Reset view to default |
| **Shift+Double-Click** | Pop out to separate window |

## Pop-Out Windows

Both 3D scene and polar plot support pop-out via **Shift+Double-Click**. Uses FBO texture sharing to avoid NVIDIA driver crashes when reparenting QOpenGLWidget on Windows.

- `Qt::AA_ShareOpenGLContexts` enabled in `main.cpp`
- FBO resizes dynamically to match pop-out window
- Shift+Double-Click in pop-out closes it

## Coordinate System

- **World Coordinates**: Z-up (mathematical convention)
- Spherical: (radius, theta, phi) where theta=azimuth, phi=elevation
- Conversion in `RadarSiteRenderer::sphericalToCartesian()` and `RadarGLWidget::sphericalToCartesian()`

## Constants

All compile-time constants are in `Constants.h` under `RadarSim::Constants` namespace, organized into: Mathematical, Compute, BVH, Geometry, Camera, Ray Tracing, Rendering, Shadow Map, Reflection Lobe, Heat Map, Defaults, Colors, Lighting, View, and UI sub-namespaces.

## Dependencies

- **Qt 6.x**: Core, Gui, Widgets, OpenGLWidgets
- **Qt 5.x fallback**: Core, Gui, Widgets, OpenGL
- **System OpenGL**: opengl32 (Windows), GL (Linux), OpenGL.framework (macOS)

No external libraries beyond Qt - all math via `QMatrix4x4`, `QVector3D`, `QQuaternion`.

## Debugging Tips

- OpenGL debug context enabled in `main.cpp`
- Qt message pattern: `[%{time yyyy-MM-dd HH:mm:ss}]`
- Use `qDebug()` for logging
- Check `initializeOpenGLFunctions()` return value

## Known Technical Debt

1. ModelManager intersection testing is placeholder
2. No unit test coverage
3. RCS ray tracing only outputs to debug console (no UI display)
4. RCS contribution calculation not yet implemented
5. Ray visualization for debugging not implemented
6. OpenGL 4.3 required - no fallback for older hardware
7. Some shader-embedded constants cannot use C++ constexpr
