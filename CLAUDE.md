# CLAUDE.md - RadarSim Project Guide

## Project Overview

RadarSim is a Qt-based 3D radar simulation application written in C++17 with OpenGL 4.3 core rendering. It visualizes radar beams with interactive controls, featuring a tabbed UI interface and a component-based architecture supporting multiple beam types. Includes GPU-accelerated ray tracing for radar cross-section (RCS) calculations.

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

# Visual Studio 2026 with CMake integration
```

## Project Structure

```
RadarSim/
├── main.cpp                    # Entry point, OpenGL context setup
├── RadarSim.h/.cpp             # Main window with tabbed UI
├── Constants.h                 # Compile-time constants (see Constants section below)
├── GLUtils.h                   # OpenGL error checking utilities
├── Config/                     # Settings and configuration system
│   ├── AppSettings.h/.cpp      # Main settings manager with profile support
│   ├── BeamConfig.h            # Beam parameters (type, width, color, opacity)
│   ├── CameraConfig.h          # Camera state (distance, angles, focus)
│   ├── TargetConfig.h          # Target parameters (position, rotation, scale)
│   └── SceneConfig.h           # Scene options (radius, radar position, visibility)
├── RadarBeam/                  # Beam visualization module
│   ├── RadarBeam.h/.cpp        # Base beam class (VAO/VBO/shaders)
│   ├── ConicalBeam.h/.cpp      # Simple cone geometry
│   ├── PhasedArrayBeam.h/.cpp  # Array beam with side lobes
│   └── BeamController.h/.cpp   # Component wrapper for beams
├── RadarSceneWidget/           # OpenGL scene management
│   ├── RadarGLWidget.h/.cpp    # Modern QOpenGLWidget with component architecture
│   ├── SphereRenderer.h/.cpp   # Sphere/grid/axes component
│   ├── RadarSceneWidget.h/.cpp # Scene container
│   └── CameraController.h/.cpp # View/camera controls
├── ModelManager/               # 3D model handling
│   └── ModelManager.h/.cpp     # Model loading component
├── WireframeTarget/            # Solid target visualization (for RCS calculations)
│   ├── WireframeShapes.h       # WireframeType enum
│   ├── WireframeTarget.h/.cpp  # Base target class (GL_TRIANGLES solid rendering)
│   ├── WireframeTargetController.h/.cpp  # Component wrapper
│   ├── CubeWireframe.h/.cpp    # Solid cube (6 faces, 12 triangles)
│   ├── CylinderWireframe.h/.cpp# Solid cylinder (caps + sides)
│   └── AircraftWireframe.h/.cpp# Solid fighter jet model
└── RCSCompute/                 # GPU ray tracing for RCS calculations
    ├── RCSTypes.h              # GPU-aligned structs (Ray, BVHNode, Triangle, HitResult)
    ├── BVHBuilder.h/.cpp       # SAH-based Bounding Volume Hierarchy construction
    └── RCSCompute.h/.cpp       # OpenGL 4.3 compute shader ray tracer
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

WireframeTarget uses solid surface rendering (GL_TRIANGLES) with indexed drawing (EBO). Vertex format is `[x, y, z, nx, ny, nz]` where the normal vector supports diffuse + ambient lighting. Face culling (`GL_CULL_FACE`) ensures only outside surfaces are drawn. Geometry is designed for radar cross-section (RCS) calculations.

### RCSCompute (GPU Ray Tracing)

GPU-accelerated ray tracing module for computing radar cross-section (RCS) via beam-target intersection. Uses OpenGL 4.3 compute shaders.

**Architecture:**
- **Ray Generation Shader**: Creates cone-distributed rays from radar position toward target
- **BVH Traversal Shader**: Tests rays against target geometry using Bounding Volume Hierarchy

**Data Structures (GPU-aligned):**
```cpp
struct Ray       { vec4 origin, direction; };           // 32 bytes
struct BVHNode   { vec4 boundsMin, boundsMax; };        // 32 bytes
struct Triangle  { vec4 v0, v1, v2; };                  // 48 bytes
struct HitResult { vec4 hitPoint, normal; uint ids; };  // 32 bytes
```

**BVH Construction:**
- Surface Area Heuristic (SAH) for optimal tree building
- 12-bin binning for split finding
- Max leaf size: 4 triangles

**Integration:**
```cpp
// In RadarGLWidget::paintGL()
rcsCompute_->setTargetGeometry(target->getVertices(), target->getIndices(), modelMatrix);
rcsCompute_->setRadarPosition(radarPos);
rcsCompute_->setBeamDirection(-radarPos.normalized());
rcsCompute_->compute();
// Results: getHitCount(), getOcclusionRatio()
```

**Current Output:** Hit count and occlusion ratio logged to debug console every 60 frames.

**Future Work:** Calculate actual RCS values from hit geometry, add UI display, visualize ray hits.

### Settings/Configuration System

The application includes a complete settings system for saving/restoring state between sessions and managing named configuration profiles.

**Architecture:**
```
Constants.h           - Compile-time constants (not user-configurable)
Config/
├── AppSettings       - Main settings manager, aggregates all configs
├── BeamConfig        - Beam parameters (type, width, opacity, color)
├── CameraConfig      - Camera state (distance, azimuth, elevation, focus)
├── TargetConfig      - Target parameters (type, position, rotation, scale)
└── SceneConfig       - Scene options (sphere radius, radar position)
```

**Features:**
- **Session Persistence**: State auto-saved on exit, restored on next launch
- **Named Profiles**: Save/load multiple named configurations
- **Profile Management UI**: Configuration tab has Save/Save As/Delete/Reset buttons

**File Storage:**
- Location: `%APPDATA%/RadarSim/` (Windows) or `~/.config/RadarSim/` (Linux/macOS)
- `last_session.json` - Auto-saved session state
- `profiles/*.json` - Named configuration profiles

**JSON Format:**
```json
{
    "version": 1,
    "beam": { "type": 0, "width": 15.0, "opacity": 0.3 },
    "camera": { "distance": 300.0, "azimuth": 0.0, "elevation": 0.4 },
    "target": { "position": [0,0,0], "rotation": [0,0,0], "scale": 20.0 },
    "scene": { "sphereRadius": 100.0, "radarTheta": 45.0, "radarPhi": 45.0 }
}
```

**Usage in RadarSim.cpp:**
```cpp
// Constructor - restore last session
if (appSettings_->restoreLastSession()) {
    QTimer::singleShot(100, this, &RadarSim::applySettingsToScene);
}

// On close - save session
void RadarSim::closeEvent(QCloseEvent* event) {
    readSettingsFromScene();
    appSettings_->saveLastSession();
}
```

**Namespace:** Config classes use `RSConfig` namespace to avoid conflict with `RadarSim` class.

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
| RCS ray tracing | `RCSCompute/RCSCompute.cpp`, `RCSCompute/BVHBuilder.cpp` |
| Add compile-time constant | `Constants.h` |
| Add saved setting | `Config/*Config.h`, `AppSettings.cpp`, `RadarSim.cpp` (read/apply methods) |
| Profile management | `Config/AppSettings.cpp`, `RadarSim.cpp` (profile slots) |

## Rendering Pipeline

```
RadarGLWidget::paintGL()
  ├── Clear buffers (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
  ├── SphereRenderer::render(projection, view, model)
  ├── ModelManager::render(projection, view, model)
  ├── WireframeTargetController::render(projection, view, model)  # Solid target
  ├── RCSCompute::compute()                                       # GPU ray tracing + shadow map
  │   └── Generates shadow map texture from ray hit distances
  ├── BeamController::render(projection, view, model)             # Semi-transparent, GPU shadow
  │   └── Fragment shader samples shadow map, discards behind hits
  └── (implicit buffer swap)
```

Note: Solid targets render before beam so they are visible through semi-transparent beam. Target rendering explicitly sets depth test, disables blending, and enables face culling for correct opaque solid rendering. RCSCompute generates both RCS data and shadow map texture which BeamController uses for accurate beam occlusion.

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

## Shadow Systems

### GPU Ray-Traced Shadow (Primary - Working)

The beam uses GPU ray-traced shadows from the RCSCompute module. This provides accurate shadow visualization that matches the RCS calculation since both use the same rays.

**How It Works:**
1. RCSCompute generates 10,000 rays in a cone pattern from radar toward target
2. BVH traversal finds ray-triangle intersections
3. Shadow map texture (64x157) stores hit distances for each ray
4. Beam fragment shader samples shadow map and discards fragments behind hit points

**Shadow Map Format:**
- Dimensions: 64 (azimuth) × 157 (elevation rings)
- Value: Hit distance (positive = hit at distance, -1.0 = miss/no shadow)
- Fragment shader compares its distance to hit distance to determine visibility

**Key Implementation Details:**

```cpp
// In RCSCompute::dispatchShadowMapGeneration()
// Store hit distance in shadow map
float hitDistance = hit.hitPoint.w;  // -1 for miss, positive for hit
imageStore(shadowMap, texCoord, vec4(hitDistance, 0.0, 0.0, 1.0));

// IMPORTANT: Unbind image after compute to allow texture sampling
glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
```

```glsl
// In RadarBeam fragment shader
if (gpuShadowEnabled) {
    vec2 uv = worldToShadowMapUV(LocalPos);
    if (uv.y >= 0.0 && uv.y <= 1.0) {
        float hitDistance = texture(shadowMap, uv).r;
        if (hitDistance > 0.0) {
            float fragDistance = length(LocalPos - radarPos);
            if (fragDistance > hitDistance) {
                discard;  // Fragment is behind hit point
            }
        }
    }
}
```

**Files Involved:**

| File | Shadow-Related Code |
|------|---------------------|
| `RCSCompute/RCSCompute.cpp` | Shadow map texture, compute shader, `dispatchShadowMapGeneration()` |
| `RCSCompute/RCSCompute.h` | `getShadowMapTexture()`, `hasShadowMap()`, `shadowMapReady_` flag |
| `RadarBeam/RadarBeam.cpp` | Fragment shader UV calculation, distance comparison |
| `RadarBeam/BeamController.cpp` | Pass-through methods for shadow map parameters |
| `RadarSceneWidget/RadarGLWidget.cpp` | Connects RCSCompute shadow map to BeamController |

**Critical Notes:**
- Must unbind image texture after compute shader writes, before fragment shader samples
- Shadow map initialized to -1.0 (no hit) to show beam before first compute
- `shadowMapReady_` flag prevents using shadow before first compute completes
- Uses `LocalPos` (untransformed vertex position) for UV calculation to match ray coordinates

### RCS Ray Tracing Future Work

See [RCSCompute (GPU Ray Tracing)](#rcscompute-gpu-ray-tracing) section for implementation details. Remaining work:

1. **Calculate actual RCS values** - Use hit geometry and material properties
2. **Add UI display** - Show hit count and occlusion ratio in application
3. **Visualize ray hits** - Debug rendering of traced rays and hit points
4. **Implement diffraction effects** - For realistic radar simulation

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

1. ModelManager intersection testing is placeholder
2. No unit test coverage
3. RCS ray tracing only outputs to debug console (no UI display)
4. RCS contribution calculation not yet implemented (placeholder in HitResult)
5. Ray visualization for debugging not implemented
6. OpenGL 4.3 required - no fallback for older hardware
7. Some shader-embedded constants cannot use C++ constexpr (workgroup size, epsilon values)

## Future Enhancements

### Beam Transparency Through Shadow (Nice to Have)

**Problem**: When viewing the beam shadow through the semi-transparent beam sides, the shadow is obscured. The fresnel effect makes glancing-angle surfaces MORE opaque, which is counterproductive for seeing through to the shadow.

**Best Attempted Solution (Option C - Inverted Fresnel)**:
```glsl
// Current (edges opaque):
float fresnel = 0.3 + 0.7 * pow(1.0 - abs(dot(norm, viewDir)), 2.0);

// Inverted (edges transparent, glass-like):
float fresnel = 1.0 - 0.5 * pow(1.0 - abs(dot(norm, viewDir)), 2.0);
```

**Location**: `RadarBeam/RadarBeam.cpp`, fragment shader (~line 128)

**Notes**: Testing showed minimal visual improvement. May require more aggressive changes to opacity, rim lighting removal, or different blending modes to achieve the desired effect.

## Mouse Controls

The 3D scene uses orbit camera controls via `CameraController`:

| Button | Action |
|--------|--------|
| **Left Mouse** | Rotate/orbit the scene (drag to rotate view around focus point) |
| **Middle Mouse** | Pan the scene (drag to move view left/right/up/down) |
| **Scroll Wheel** | Zoom in/out (changes camera distance) |
| **Left Double-Click** | Reset view to default position |

**Implementation Details** (`CameraController.cpp`):
- Orbit uses spherical coordinates (azimuth, elevation) around focus point
- Pan moves the focus point in screen-space aligned directions
- Inertia can be enabled for smooth rotation continuation after mouse release
- Elevation clamped to ±85° to avoid gimbal lock

## Coordinate System

- **World Coordinates**: Z-up (mathematical convention)
- Spherical coordinates: (radius, theta, phi)
  - `theta`: azimuth angle (horizontal rotation)
  - `phi`: elevation angle (vertical rotation)
- Conversion to Cartesian in `SphereRenderer::updateRadarDotPosition()`

## Constants.h Organization

All compile-time constants are centralized in `Constants.h` under the `RadarSim::Constants` namespace:

```cpp
namespace RadarSim::Constants {
    // Compute Shader Configuration
    kComputeWorkgroupSize, kDefaultNumRays, kRaysPerRing

    // BVH Settings
    kBVHMaxLeafSize, kBVHNumBins, kBVHStackSize

    // Geometry Generation
    kBeamConeSegments, kBeamCapRings, kCylinderSegments,
    kSphereLatSegments, kSphereLongSegments, kAxisArrowSegments

    // Camera Limits
    kCameraMinDistance, kCameraMaxDistance, kCameraMaxElevation,
    kCameraRotationSpeed, kCameraZoomSpeed, kCameraInertiaDecay

    // Ray Tracing
    kRayTMinEpsilon, kTriangleEpsilon, kMaxRayDistanceMultiplier

    // Rendering
    kGridLineWidthNormal, kGridLineWidthSpecial, kGridRadiusOffset,
    kRadarDotRadius, kRadarDotVertices

    // Shader Visual Constants
    kFresnelMin, kFresnelMax, kMinBeamOpacity,
    kAmbientStrength, kSpecularStrength, kShininess

    // Shadow Map
    kShadowMapWidth, kShadowMapHeight, kShadowMapNoHit

    namespace Defaults {
        kCameraDistance, kCameraAzimuth, kCameraElevation,
        kSphereRadius, kRadarTheta, kRadarPhi,
        kBeamWidth, kBeamOpacity, kTargetScale
    }

    namespace Colors {
        kBackgroundGrey, kGridLineGrey, kSphereOffWhite,
        kBeamOrange, kTargetGreen,
        kAxisRed, kAxisGreen, kAxisBlue,
        kEquatorGreen, kPrimeMeridianBlue
    }

    namespace Lighting {
        kLightPosition, kTargetLightPosition
    }

    namespace View {
        kPerspectiveFOV, kNearPlane, kFarPlane, kAxisLengthMultiplier
    }

    namespace UI {
        kAxisLabelFontSize, kTextOffsetPixels
    }
}
```

**Usage**: Include `Constants.h` and add `using namespace RadarSim::Constants;` to access constants directly.
