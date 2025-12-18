# CLAUDE.md - RadarSim Project Guide

## Project Overview

RadarSim is a Qt-based 3D radar simulation application written in C++17 with OpenGL 3.3 core rendering. It visualizes radar beams with interactive controls, featuring a tabbed UI interface and a component-based architecture supporting multiple beam types.

- **Qt Version**: 6.10.1 (with Qt5 fallback support)
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
│   ├── PhasedArrayBeam.h/.cpp  # Array beam with side lobes
│   └── BeamController.h/.cpp   # Component wrapper for beams
├── RadarSceneWidget/           # OpenGL scene management
│   ├── RadarGLWidget.h/.cpp    # Modern QOpenGLWidget (primary)
│   ├── SphereWidget.h/.cpp     # Legacy OpenGL widget
│   ├── SphereRenderer.h/.cpp   # Sphere/grid/axes component
│   ├── RadarSceneWidget.h/.cpp # Scene container
│   └── CameraController.h/.cpp # View/camera controls
├── ModelManager/               # 3D model handling
│   └── ModelManager.h/.cpp     # Model loading component
└── WireframeTarget/            # Solid target visualization (for RCS calculations)
    ├── WireframeShapes.h       # WireframeType enum
    ├── WireframeTarget.h/.cpp  # Base target class (GL_TRIANGULAR solid rendering)
    ├── WireframeTargetController.h/.cpp  # Component wrapper
    ├── CubeWireframe.h/.cpp    # Solid cube (6 faces, 12 triangles)
    ├── CylinderWireframe.h/.cpp# Solid cylinder (caps + sides)
    └── AircraftWireframe.h/.cpp# Solid fighter jet model
```

## Architecture

### Component Pattern

The project uses a component-based architecture where `RadarGLWidget` owns and coordinates multiple components:

- **SphereRenderer**: Renders the sphere, grid lines, axes, and radar position dot
- **BeamController**: Manages radar beam creation and rendering
- **CameraController**: Handles view transformations, mouse interaction, inertia
- **ModelManager**: Loads and renders 3D models
- **WireframeTargetController**: Manages solid target shapes with transforms (for RCS)

Components are initialized after OpenGL context is ready:
```cpp
void RadarGLWidget::initializeGL() {
    sphereRenderer_->initialize();
    beamController_->initialize();
    cameraController_->initialize();
    modelManager_->initialize();
    wireframeController_->initialize();
}
```

### Beam Hierarchy

```
RadarBeam (base)
├── ConicalBeam
└── PhasedArrayBeam
```

Factory method for beam creation:
```cpp
std::shared_ptr<RadarBeam> RadarBeam::createBeam(BeamType type);
```

### WireframeTarget Hierarchy

```
WireframeTarget (base)
├── CubeWireframe      # 6 faces, 12 triangles
├── CylinderWireframe  # Top/bottom caps + side surface
└── AircraftWireframe  # Triangulated fighter jet surfaces
```

Factory method for target creation:
```cpp
WireframeTarget* WireframeTarget::createTarget(WireframeType type);
```

WireframeTarget uses solid surface rendering (GL_TRIANGLES) with indexed drawing (EBO). Vertex format is `[x, y, z, nx, ny, nz]` where the normal vector supports diffuse + ambient lighting. Face culling (`GL_CULL_FACE`) ensures only outside surfaces are drawn. Geometry is designed for future radar cross-section (RCS) calculations.

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

### Deferred Beam Type Changes

Beam type changes from UI (context menu) must be deferred to `paintGL()` where a valid GL context exists:

```cpp
// In BeamController - defer the change
void setBeamType(BeamType type) {
    pendingBeamType_ = type;
    beamTypeChangePending_ = true;
}

// In rebuildBeamGeometry() called from paintGL()
void rebuildBeamGeometry() {
    if (beamTypeChangePending_) {
        currentBeamType_ = pendingBeamType_;
        beamTypeChangePending_ = false;
        createBeam();  // Now we have GL context for proper cleanup
    }
    // ... upload geometry
}
```

**Why**: Deleting a beam outside GL context leaks OpenGL resources (VAO/VBO/EBO). The destructor needs a valid context to call `glDeleteBuffers()`.

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
| Add new solid target | `WireframeTarget/WireframeShapes.h` (enum), new class inheriting `WireframeTarget` |
| Modify UI controls | `RadarSim.cpp` (`setupTabs()`, slot methods) |
| Change rendering | `RadarGLWidget.cpp` (`paintGL()`), component `render()` methods |
| Adjust camera | `CameraController.cpp` |
| Sphere geometry | `SphereRenderer.cpp` |
| Target transforms | `WireframeTargetController.cpp`, `RadarSim.cpp` (target slots) |

## Rendering Pipeline

```
RadarGLWidget::paintGL()
  ├── Clear buffers (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
  ├── SphereRenderer::render(projection, view, model)
  ├── ModelManager::render(projection, view, model)
  ├── WireframeTargetController::render(projection, view, model)  # Solid, opaque
  ├── BeamController::render(projection, view, model)             # Semi-transparent
  └── (implicit buffer swap)
```

Note: Solid targets render before beam so they are visible through semi-transparent beam. Target rendering explicitly sets depth test, disables blending, and enables face culling for correct opaque solid rendering.

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
enum class BeamType { Conical, Shaped, Phased, NewType };
```

2. Create new class inheriting from `RadarBeam`:
```cpp
class NewBeam : public RadarBeam {
    void generateGeometry() override;
};
```

3. Update factory in `RadarBeam::createBeam()`

### Adding a New Solid Target Type

1. Add enum value in `WireframeShapes.h`:
```cpp
enum class WireframeType { Cube, Cylinder, Aircraft, NewShape };
```

2. Create new class inheriting from `WireframeTarget`:
```cpp
class NewShapeWireframe : public WireframeTarget {
protected:
    void generateGeometry() override {
        clearGeometry();

        // Define vertices with positions and normals
        GLuint baseIdx = getVertexCount();
        addVertex(QVector3D(x, y, z), QVector3D(nx, ny, nz));  // position, normal
        // ... more vertices

        // Define triangles using vertex indices
        addTriangle(idx0, idx1, idx2);
        // Or quads (converted to 2 triangles internally)
        addQuad(idx0, idx1, idx2, idx3);
    }
};
```

3. Update factory in `WireframeTarget::createTarget()`

4. Add context menu action in `RadarGLWidget::setupContextMenu()`

**Note**: Ensure correct winding order (counter-clockwise when viewed from outside) for face culling to work properly. Normals should point outward for correct lighting.

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

## Shadow Volume / Beam Occlusion (Work in Progress)

The system implements shadow volumes using the Z-fail (Carmack's Reverse) stencil algorithm to occlude the radar beam behind solid targets. This is foundational work for future radar cross-section (RCS) calculations.

### Current Status

**Working:**
- Shadow volume generation from target geometry
- Beam occlusion through solid targets (beam not visible inside target)
- Shadow follows radar position changes (azimuth, elevation, radius sliders)

**Not Working:**
- Shadow does NOT follow whole-scene rotations (mouse drag rotation)
- Depth cap disabled due to visibility issues

### Implementation Details

**Components:**
- **Shadow Volume Generation** (`WireframeTarget::generateShadowVolume`): For each front-facing triangle of the target, extrudes vertices away from the radar position to create a closed shadow volume (front cap, back cap, side walls).
- **Depth Cap** (DISABLED): Was intended to provide far-plane depth values for Z-fail algorithm, but caused target/beam to become invisible. Code remains but is commented out.
- **Stencil Rendering** (`renderShadowVolume`): Two-pass rendering with face culling to increment/decrement stencil buffer.

**Render Flow:**
```
WireframeTarget::render()
  ├── Pass 1: Render solid target (color + depth)
  ├── Pass 2: Shadow volume (stencil operations)
  │   ├── Generate shadow volume in VIEW space
  │   ├── Render back faces (stencil increment on depth fail)
  │   ├── Render front faces (stencil decrement on depth fail)
  │   └── Leave stencil test enabled for beam
  └── Beam renders with stencil test (GL_EQUAL, 0)
```

**Coordinate Space:**
- Shadow volume generated in VIEW space (`view * combinedModel` applied to vertices)
- Radar position also transformed to view space
- Rendered with projection only (view/model identity since already in view space)

### Known Issues

1. **Scene Rotation**: Shadow does not follow when the whole scene is rotated via mouse drag. The coordinate space transformation between radar position and shadow volume is not correctly accounting for scene rotation in all cases.

2. **Depth Cap Disabled**: The depth cap sphere (needed for proper Z-fail algorithm) causes the target and beam to become invisible. Multiple approaches tried (different culling modes, render order) but none worked correctly. Without the depth cap, the Z-fail algorithm may not work correctly for all viewing angles.

3. **Z-Fail Limitations**: The Z-fail algorithm requires proper far-plane geometry (depth cap) to work correctly. Without it, shadow may disappear or behave incorrectly when camera is inside the shadow volume.

### Files Involved

| File | Shadow-Related Code |
|------|---------------------|
| `WireframeTarget.h` | Shadow VAO/VBO/EBO, depth cap resources, method declarations |
| `WireframeTarget.cpp` | `generateShadowVolume()`, `renderShadowVolume()`, `generateDepthCap()`, `renderDepthCap()` (disabled), `setupShadowShaders()` |
| `RadarBeam.cpp` | Stencil test in `render()` method |
| `RadarGLWidget.cpp` | Clears stencil buffer, passes radar position and sphere radius |

### Future Work

1. **Fix scene rotation tracking** - Investigate why shadow doesn't follow mouse-drag scene rotation
2. **Fix depth cap** - Resolve visibility issues so Z-fail algorithm works correctly
3. **Consider Z-pass alternative** - Z-pass algorithm doesn't need depth cap but fails when camera is inside shadow
4. **Implement diffraction effects** - For realistic radar simulation
5. **Use shadow geometry for RCS calculations** - Leverage shadow volume for radar cross-section

## UI Layout

### Control Group Box Pattern

Both Radar Controls and Target Controls use compact layout:
```cpp
QVBoxLayout* layout = new QVBoxLayout(groupBox);
layout->setSpacing(2);                    // Minimal vertical spacing
layout->setContentsMargins(6, 6, 6, 6);   // Compact margins
```

Each control follows this pattern:
```cpp
QLabel* label = new QLabel("Control Name", groupBox);
layout->addWidget(label);

QHBoxLayout* controlLayout = new QHBoxLayout();
QSlider* slider = new QSlider(Qt::Horizontal, groupBox);
QSpinBox* spinBox = new QSpinBox(groupBox);
controlLayout->addWidget(slider);
controlLayout->addWidget(spinBox);
layout->addLayout(controlLayout);
```

### Target Controls Slots

Target control slots must call `radarSceneView_->update()` after modifying the controller:
```cpp
void RadarSim::onTargetPosXChanged(int value) {
    // Sync slider and spinbox with blockSignals()
    // ...

    if (auto* controller = radarSceneView_->getWireframeController()) {
        QVector3D pos = controller->getPosition();
        controller->setPosition(static_cast<float>(value), pos.y(), pos.z());
        radarSceneView_->update();  // IMPORTANT: Trigger repaint
    }
}
```

## Known Technical Debt

1. Dual rendering paths (legacy SphereWidget + new component system)
2. Mixed memory management (raw `new`/`delete` and `std::shared_ptr`)
3. ModelManager intersection testing is placeholder
4. No unit test coverage
5. Shadow volume scene rotation has edge cases (see Shadow Volume section)
6. Excessive qDebug() logging in paintGL() should be reduced for release

## Coordinate System

- Spherical coordinates: (radius, theta, phi)
  - `theta`: azimuth angle (horizontal rotation)
  - `phi`: elevation angle (vertical rotation)
- Conversion to Cartesian in `SphereRenderer::updateRadarDotPosition()`
