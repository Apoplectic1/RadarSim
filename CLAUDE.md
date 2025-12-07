# CLAUDE.md - RadarSim Project Guide

## Project Overview

RadarSim is a Qt-based 3D radar simulation application written in C++17 with OpenGL 3.3 core rendering. It visualizes radar beams with interactive controls, featuring a tabbed UI interface and a component-based architecture supporting multiple beam types.

- **Qt Version**: 6.10.0 (with Qt5 fallback support)
- **OpenGL**: 3.3 Core Profile
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

# Or use Visual Studio with CMake integration
```

## Project Structure

```
RadarSim/
├── main.cpp                    # Entry point, OpenGL context setup
├── RadarSim.h/.cpp             # Main window with tabbed UI
├── RadarBeam/                  # Beam visualization module
│   ├── RadarBeam.h/.cpp        # Base beam class (VAO/VBO/shaders)
│   ├── ConicalBeam.h/.cpp      # Simple cone geometry
│   ├── EllipticalBeam.h/.cpp   # Different H/V beam widths
│   ├── PhasedArrayBeam.h/.cpp  # Array beam with side lobes
│   └── BeamController.h/.cpp   # Component wrapper for beams
├── RadarSceneWidget/           # OpenGL scene management
│   ├── RadarGLWidget.h/.cpp    # Modern QOpenGLWidget (primary)
│   ├── SphereWidget.h/.cpp     # Legacy OpenGL widget
│   ├── SphereRenderer.h/.cpp   # Sphere/grid/axes component
│   ├── RadarSceneWidget.h/.cpp # Scene container
│   └── CameraController.h/.cpp # View/camera controls
└── ModelManager/               # 3D model handling
    └── ModelManager.h/.cpp     # Model loading component
```

## Architecture

### Component Pattern

The project uses a component-based architecture where `RadarGLWidget` owns and coordinates multiple components:

- **SphereRenderer**: Renders the sphere, grid lines, axes, and radar position dot
- **BeamController**: Manages radar beam creation and rendering
- **CameraController**: Handles view transformations, mouse interaction, inertia
- **ModelManager**: Loads and renders 3D models

Components are initialized after OpenGL context is ready:
```cpp
void RadarGLWidget::initializeGL() {
    sphereRenderer_->initialize();
    beamController_->initialize();
    cameraController_->initialize();
    modelManager_->initialize();
}
```

### Beam Hierarchy

```
RadarBeam (base)
├── ConicalBeam
├── EllipticalBeam
└── PhasedArrayBeam
```

Factory method for beam creation:
```cpp
std::shared_ptr<RadarBeam> RadarBeam::createBeam(BeamType type);
```

### Dual Implementation (Transition State)

The codebase has both legacy and new implementations:
- **Legacy**: `SphereWidget` - standalone OpenGL widget
- **New**: `RadarGLWidget` + components - modular architecture
- A UI toggle in Configuration tab switches between them

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
- Use `QOpenGLFunctions_3_3_Core` for type-safe GL calls
- VAO/VBO/EBO pattern for geometry
- Shaders defined as raw strings in constructors
- `QMatrix4x4` for all matrix operations

### Dynamic Geometry Buffers

**Important**: For geometry that changes at runtime (e.g., beam shape when radius changes), use raw OpenGL buffer IDs instead of `QOpenGLBuffer`:

```cpp
// In header - use raw GLuint instead of QOpenGLBuffer
GLuint vboId_ = 0;
GLuint eboId_ = 0;

// Creating/updating buffers
if (vboId_ == 0) {
    glGenBuffers(1, &vboId_);
}
glBindBuffer(GL_ARRAY_BUFFER, vboId_);
glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);

// Cleanup in destructor
if (vboId_ != 0) {
    glDeleteBuffers(1, &vboId_);
    vboId_ = 0;
}
```

**Why**: `QOpenGLBuffer::allocate()` can only be called once per buffer. Mixing `QOpenGLBuffer` methods with raw `glBufferData()` causes state inconsistencies and crashes. Raw OpenGL gives full control for dynamic updates.

### OpenGL Context Requirements

- Never call OpenGL functions (including `initialize()`) from constructors - the GL context isn't ready
- Use a `geometryDirty_` flag to defer GPU uploads when setters are called without a valid context
- Check `QOpenGLContext::currentContext()` before any GL operations outside render methods
- `BeamController::createBeam()` handles calling `initialize()` after construction with valid context

### Signal Connections for Animation

Components that update asynchronously (e.g., inertia timers) must have their signals connected to trigger widget repaints:

```cpp
// In RadarGLWidget::initialize()
connect(cameraController_, &CameraController::viewChanged,
        this, QOverload<>::of(&QWidget::update));
```

Without this connection, timer-based animations will appear stuttery because `update()` is never called.

## Key Files for Common Tasks

| Task | Files |
|------|-------|
| Add new beam type | `RadarBeam/RadarBeam.h` (enum), new class inheriting `RadarBeam` |
| Modify UI controls | `RadarSim.cpp` (`setupTabs()`, slot methods) |
| Change rendering | `RadarGLWidget.cpp` (`paintGL()`), component `render()` methods |
| Adjust camera | `CameraController.cpp` |
| Sphere geometry | `SphereRenderer.cpp` |

## Rendering Pipeline

```
RadarGLWidget::paintGL()
  ├── Clear buffers (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
  ├── SphereRenderer::render(projection, view)
  ├── BeamController::render(projection, view, model)
  ├── ModelManager::render(projection, view)
  └── (implicit buffer swap)
```

## Common Modifications

### Adding a UI Control

1. Add widget in `RadarSim::setupTabs()`:
```cpp
auto* slider = new QSlider(Qt::Horizontal);
auto* spinBox = new QDoubleSpinBox();
```

2. Connect signals with bidirectional sync:
```cpp
connect(slider, &QSlider::valueChanged, this, &RadarSim::onNewSliderChanged);
connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        this, &RadarSim::onNewSpinBoxChanged);
```

3. Implement slots with signal blocking:
```cpp
void RadarSim::onNewSliderChanged(int value) {
    spinBox_->blockSignals(true);
    spinBox_->setValue(value / 10.0);
    spinBox_->blockSignals(false);
    // Update scene
}
```

### Adding a New Beam Type

1. Add enum value in `RadarBeam.h`:
```cpp
enum class BeamType { Conical, Elliptical, Shaped, Phased, NewType };
```

2. Create new class inheriting from `RadarBeam`:
```cpp
class NewBeam : public RadarBeam {
    void generateGeometry() override;
};
```

3. Update factory in `RadarBeam::createBeam()`

### Adding a Component

1. Create header/source in appropriate directory
2. Add to `CMakeLists.txt` source lists
3. Include in `RadarSceneWidget::createComponents()`
4. Initialize in `RadarGLWidget::initializeGL()`
5. Render in `RadarGLWidget::paintGL()`

## Dependencies

- **Qt 6.x**: Core, Gui, Widgets, OpenGLWidgets
- **Qt 5.x fallback**: Core, Gui, Widgets, OpenGL
- **System OpenGL**: opengl32 (Windows), GL (Linux), OpenGL.framework (macOS)

No external libraries beyond Qt - all math via `QMatrix4x4`, `QVector3D`, `QQuaternion`.

## Debugging Tips

- OpenGL debug context is enabled in `main.cpp`
- Qt message pattern includes timestamps: `[%{time yyyy-MM-dd HH:mm:ss}]`
- Use `qDebug()` for logging (output formatted per pattern)
- Check `initializeOpenGLFunctions()` return value for GL issues

## Known Technical Debt

1. Dual rendering paths (legacy SphereWidget + new component system)
2. Mixed memory management (raw `new`/`delete` and `std::shared_ptr`)
3. ModelManager intersection testing is placeholder
4. No unit test coverage

## Coordinate System

- Spherical coordinates: (radius, theta, phi)
  - `theta`: azimuth angle (horizontal rotation)
  - `phi`: elevation angle (vertical rotation)
- Conversion to Cartesian in `SphereRenderer::updateRadarDotPosition()`
