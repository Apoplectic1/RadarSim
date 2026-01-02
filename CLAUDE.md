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

# Visual Studio 2026 with CMake integration
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
└── PolarPlot/                  # 2D polar RCS visualization
    └── PolarRCSPlot.h/.cpp     # OpenGL widget for polar RCS plot
```

## Architecture

### Component Pattern

The project uses a component-based architecture where `RadarGLWidget` owns and coordinates multiple components:

- **SphereRenderer**: Renders the sphere, grid lines, and axes
- **RadarSiteRenderer**: Renders the radar site position dot on the sphere
- **BeamController**: Manages radar beam creation and rendering (Conical, Sinc, Phased)
- **CameraController**: Handles view transformations, mouse interaction, inertia
- **ModelManager**: Loads and renders 3D models
- **WireframeTargetController**: Manages solid target shapes with transforms (for RCS)
- **RCSCompute**: GPU ray tracing for radar cross-section calculations
- **ReflectionRenderer**: Visualizes RCS as colored cone lobes from hit points
- **HeatMapRenderer**: Visualizes RCS as smooth gradient heat map on radar sphere

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

### Control Widget Pattern

UI control panels are implemented as self-contained widget classes in `Controls/`:

- **RadarControlsWidget**: Radar sphere radius and position (theta, phi)
- **TargetControlsWidget**: Target position, rotation, and scale
- **RCSPlaneControlsWidget**: RCS cut type, plane offset, and slice thickness

Each widget encapsulates:
1. **UI setup**: Creates QSlider, QSpinBox, QComboBox, QCheckBox as needed
2. **Internal synchronization**: Slider/spinbox values stay in sync via `blockSignals()`
3. **Double-click reset**: Event filter resets sliders to defaults
4. **Settings persistence**: `readSettings()` and `applySettings()` methods for save/restore

Widgets emit signals when values change, which `RadarSim` connects to the scene:
```cpp
// In RadarSim::connectSignals()
connect(radarControls_, &RadarControlsWidget::radiusChanged,
        this, &RadarSim::onRadarRadiusChanged);
connect(targetControls_, &TargetControlsWidget::positionChanged,
        this, &RadarSim::onTargetPositionChanged);
```

### Beam Hierarchy

```
RadarBeam (base)
├── ConicalBeam      # Uniform intensity cone
├── PhasedArrayBeam  # Main lobe + side lobes (phased array pattern)
└── SincBeam         # Sinc² intensity pattern with natural side lobes
```

**BeamType enum:** `Conical`, `Shaped`, `Phased`, `Sinc`

**SincBeam Details:**
- Intensity follows sinc²(πθ/θmax) pattern (realistic electromagnetic field)
- 7-float vertex format: `[x, y, z, nx, ny, nz, intensity]`
- Extended geometry (4× main lobe via `kSincSideLobeMultiplier`) for visible side lobes
- Per-vertex intensity drives color blending and alpha modulation
- Side lobes at ~1.43× (−13dB) and ~2.46× (−18dB) main lobe angle
- `getVisualExtentMultiplier()` returns 4.0 for proper shadow ray coverage

Factory method for beam creation:
```cpp
RadarBeam* RadarBeam::createBeam(BeamType type, float sphereRadius, float beamWidthDegrees);
```

### WireframeTarget Hierarchy

```
WireframeTarget (base)
├── CubeWireframe      # 6 faces, 12 triangles
├── CylinderWireframe  # Top/bottom caps + side surface
├── AircraftWireframe  # Triangulated fighter jet surfaces
└── SphereWireframe    # Geodesic sphere (icosahedron subdivision, 1280 faces default)
```

**SphereWireframe Details:**
- Created via recursive icosahedron subdivision
- Subdivision level configurable (0=20 faces, 1=80, 2=320, 3=1280)
- Used for RCS verification against theoretical πr²
- Normals point outward from sphere center

Factory method for target creation:
```cpp
std::unique_ptr<WireframeTarget> WireframeTarget::createTarget(WireframeType type);
```

WireframeTarget uses solid surface rendering (GL_TRIANGLES) with indexed drawing (EBO). Vertex format is `[x, y, z, nx, ny, nz]` where the normal vector supports diffuse + ambient lighting. Face culling (`GL_CULL_FACE`) ensures only outside surfaces are drawn. Geometry is designed for radar cross-section (RCS) calculations.

### RCSCompute (GPU Ray Tracing)

GPU-accelerated ray tracing module for computing radar cross-section (RCS) via beam-target intersection. Uses OpenGL 4.3 compute shaders.

**Architecture:**
- **Ray Generation Shader**: Creates cone-distributed rays from radar position toward target
- **BVH Traversal Shader**: Tests rays against target geometry using Bounding Volume Hierarchy

**Data Structures (GPU-aligned):**
```cpp
struct Ray       { vec4 origin, direction; };                    // 32 bytes
struct BVHNode   { vec4 boundsMin, boundsMax; };                  // 32 bytes
struct Triangle  { vec4 v0, v1, v2; };                            // 48 bytes
struct HitResult { vec4 hitPoint, normal, reflection; uint ids; };// 64 bytes
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

**Future Work:** Calculate actual RCS values from hit geometry, add UI display.

### ReflectionRenderer (Lobe Visualization)

Visualizes RCS reflected energy as colored cone lobes emanating from target surface.

**Features:**
- Colored cones show reflection directions from ray-target hits
- Color gradient: Red (high intensity) → Yellow → Blue (low intensity)
- BRDF-based intensity calculation (diffuse + specular)
- CPU-side clustering aggregates ~10K hits into ~100 lobes
- Toggle via Configuration window ("Show Reflection Lobes" checkbox)

**Clustering Algorithm:**
- Groups hits by spatial proximity (kLobeClusterDist = 5 units)
- Groups by angular similarity (kLobeClusterAngle = 10°)
- Weighted average of position, direction, and intensity
- Filters low-intensity lobes (< 5% threshold)

**Rendering:**
- Cone geometry per lobe (12 segments)
- Alpha blending for transparency (opacity = 0.7)
- Fresnel-based edge glow effect
- Rendered last in pipeline (after beam)

**Integration:**
```cpp
// In RadarGLWidget::paintGL()
rcsCompute_->compute();
rcsCompute_->readHitBuffer();  // GPU → CPU transfer
reflectionRenderer_->updateLobes(rcsCompute_->getHitResults());
reflectionRenderer_->render(projection, view, model);
```

### HeatMapRenderer (Sphere Heat Map)

Alternative RCS visualization showing reflection intensity as a smooth gradient heat map on the radar sphere surface.

**Features:**
- Maps reflection directions to spherical coordinates on radar sphere
- Blue (low) → Yellow (mid) → Red (high) intensity gradient
- Smooth per-vertex interpolation for gradient appearance
- Spherical binning (1024×1024 lat/lon bins) for intensity accumulation
- **Slice filtering**: Only shows hits within current plane offset and thickness (synced with polar plot)
- Toggle via Configuration window ("Show RCS Heat Map" checkbox)

**Coordinate Mapping:**
```cpp
// Reflection direction → spherical coordinates
QVector3D dir = hit.reflection.toVector3D().normalized();
float theta = atan2(dir.y(), dir.x());   // azimuth [-π, π]
float phi = asin(dir.z());               // elevation [-π/2, π/2]

// Spherical coords → bin index (north pole = lat 0, south pole = lat max)
int latBin = (π/2 - phi) / π * latBins;  // +π/2 → 0, -π/2 → max
int lonBin = (theta + π) / 2π * lonBins;
```

**Rendering:**
- Sphere mesh at `radius * 1.02` (slightly above main sphere)
- Alpha blending with intensity-modulated opacity
- Rendered after sphere, before beam for proper layering

**Integration:**
```cpp
// In RadarGLWidget::paintGL()
if (heatMapRenderer_ && heatMapRenderer_->isVisible()) {
    heatMapRenderer_->updateFromHits(rcsCompute_->getHitResults(), radius_);
    heatMapRenderer_->render(projection, view, model);
}
```

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
└── SceneConfig       - Scene options (sphere radius, radar position, visibility toggles)
```

**Features:**
- **Session Persistence**: State auto-saved on exit, restored on next launch
- **Named Profiles**: Save/load multiple named configurations
- **Profile Management UI**: Floating Configuration window has Save/Save As/Delete/Reset buttons

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
    "target": { "type": 0, "position": [0,0,0], "rotation": [0,0,0], "scale": 20.0 },
    "scene": {
        "sphereRadius": 100.0, "radarTheta": 45.0, "radarPhi": 45.0,
        "showSphere": true, "showAxes": true, "showGridLines": true,
        "showShadow": true, "enableInertia": false,
        "rcsCutType": 0, "rcsPlaneOffset": 0.0, "rcsSliceThickness": 10.0,
        "rcsPlaneShowFill": true
    }
}
```

**Target Types:** 0=Cube, 1=Cylinder, 2=Aircraft, 3=Sphere
**RCS Cut Types:** 0=Azimuth, 1=Elevation

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
- Use `QOpenGLFunctions_4_5_Core` for type-safe GL calls
- VAO/VBO/EBO pattern for geometry
- Shader sources stored as `std::string_view` members (type-safe string literals)
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
| Add new beam type | `RadarBeam/RadarBeam.h` (enum), new class inheriting `RadarBeam`, override `getVisualExtentMultiplier()` if beam has side lobes |
| Add new solid target | `WireframeTarget/WireframeShapes.h` (enum), new class inheriting `WireframeTarget` |
| Modify radar controls | `Controls/RadarControlsWidget.cpp` (UI, signals, settings), `RadarSim.cpp` (connect to scene) |
| Modify target controls | `Controls/TargetControlsWidget.cpp` (UI, signals, settings), `RadarSim.cpp` (connect to scene) |
| Modify RCS plane controls | `Controls/RCSPlaneControlsWidget.cpp` (UI, signals, settings), `RadarSim.cpp` (connect to scene) |
| Configuration options | `ConfigurationWindow.cpp`, `RadarSim.cpp` (connect signals) |
| Floating windows | `ControlsWindow.cpp`, `ConfigurationWindow.cpp`, `RadarSim.cpp` (positioning, show/hide) |
| Change rendering | `RadarGLWidget.cpp` (`paintGL()`), component `render()` methods |
| Adjust camera | `CameraController.cpp` |
| Sphere geometry | `SphereRenderer.cpp` |
| Radar site position | `RadarSiteRenderer.cpp`, `RadarGLWidget.cpp` |
| Target transforms | `WireframeTargetController.cpp`, `Controls/TargetControlsWidget.cpp` |
| RCS ray tracing | `RCSCompute/RCSCompute.cpp`, `RCSCompute/BVHBuilder.cpp` |
| RCS sampling/polar plot | `RCSCompute/AzimuthCutSampler.cpp`, `RCSCompute/ElevationCutSampler.cpp`, `PolarPlot/PolarRCSPlot.cpp` |
| Slicing plane visualization | `RCSCompute/SlicingPlaneRenderer.cpp` |
| Reflection lobes | `ReflectionRenderer/ReflectionRenderer.cpp`, `RCSCompute/RCSCompute.cpp` |
| RCS heat map | `ReflectionRenderer/HeatMapRenderer.cpp`, `RCSCompute/RCSCompute.cpp` |
| Pop-out windows | `PopOutWindow.cpp`, `FBORenderer.cpp`, `TextureBlitWidget.cpp`, `RadarSim.cpp` (pop-out slots) |
| Add compile-time constant | `Constants.h` |
| Add saved setting | `Config/*Config.h`, `AppSettings.cpp`, `RadarSim.cpp` (read/apply methods) |
| Profile management | `Config/AppSettings.cpp`, `RadarSim.cpp` (profile slots) |

## Event-Driven Update Model

The application uses **Qt's event-driven rendering** - there is no continuous render loop. Updates only occur when triggered:

### What Triggers a Frame

| Trigger | Source | Mechanism |
|---------|--------|-----------|
| Mouse orbit/pan | CameraController | `emit viewChanged()` → `update()` |
| Mouse zoom | CameraController | `emit viewChanged()` → `update()` |
| Slider/control change | RadarSim slots | `radarSceneView_->updateScene()` |
| Inertia animation | CameraController timer | 16ms QTimer → `emit viewChanged()` |
| Window resize/expose | Qt | Automatic repaint |

### Update Flow

```
USER ACTION (mouse drag, slider change, etc.)
    ↓
CONTROL EVENT HANDLER (slot in RadarSim.cpp)
    ↓
STATE UPDATE (e.g., camera angles, target position)
    ↓
QWidget::update() called
    ↓
Qt queues repaint event
    ↓
RadarGLWidget::paintGL() executes
    ↓
RCS compute runs INSIDE paintGL()
    ↓
Results emitted via signal to PolarRCSPlot
```

### RCS Processing

RCS computation happens **synchronously inside `paintGL()`** every frame (not in a separate thread):

```cpp
// Inside RadarGLWidget::paintGL() (~line 320-380)
if (wireframeController_ && wireframeController_->isVisible()) {
    rcsCompute_->setTargetGeometry(...);
    rcsCompute_->compute();           // GPU ray tracing
    rcsCompute_->readHitBuffer();     // GPU → CPU transfer
    currentSampler_->sample(hits, polarPlotData_);
    emit polarPlotDataReady(polarPlotData_);  // → PolarRCSPlot
}
```

### Inertia Timer (Pseudo-Continuous Loop)

When inertia is enabled, a QTimer fires every ~16ms creating continuous updates:

```cpp
// CameraController::onInertiaTimerTimeout()
azimuth_ += azimuthVelocity_;
azimuthVelocity_ *= 0.95f;  // Decay
emit viewChanged();         // Triggers paintGL()
```

When idle (no interaction, no inertia), the scene doesn't render - no wasted GPU cycles.

## Rendering Pipeline

```
RadarGLWidget::paintGL()
  ├── Clear buffers (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
  ├── SphereRenderer::render(projection, view, model)
  ├── RadarSiteRenderer::render(projection, view, model, radius)  # Radar position dot
  ├── ModelManager::render(projection, view, model)
  ├── WireframeTargetController::render(projection, view, model)  # Solid target
  ├── RCSCompute::compute()                                       # GPU ray tracing + shadow map
  │   ├── Generates shadow map texture from ray hit distances
  │   └── Calculates reflection directions and BRDF intensities
  ├── RCSCompute::readHitBuffer()                                 # GPU → CPU for visualization
  ├── SlicingPlaneRenderer::render()                              # Translucent RCS sampling plane
  ├── HeatMapRenderer::updateFromHits() + render()                # Heat map on sphere (if enabled)
  ├── BeamController::render(projection, view, model)             # Semi-transparent, GPU shadow
  │   └── Fragment shader samples shadow map, discards behind hits
  ├── ReflectionRenderer::render(projection, view, model)         # Transparent lobe cones (if enabled)
  ├── Sample RCS data → emit polarPlotDataReady signal            # For 2D polar plot
  └── (implicit buffer swap)

PolarRCSPlot (separate widget, below 3D scene)
  ├── Receives RCSDataPoint vector via signal/slot
  ├── Renders polar grid (30° lines, 10 dB rings)
  ├── Renders RCS data curve (filled polygon from center)
  └── Renders axis labels (0°/90°/180°/270° and dBsm values)
```

Note: Solid targets render before beam so they are visible through semi-transparent beam. Target rendering explicitly sets depth test, disables blending, and enables face culling for correct opaque solid rendering. RCSCompute generates both RCS data and shadow map texture which BeamController uses for accurate beam occlusion. ReflectionRenderer renders last with alpha blending for proper transparency.

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

4. Add combo box option in `ConfigurationWindow::createTargetGroup()`

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

### Show Shadow Toggle

The beam projection (cap) on the sphere surface can be toggled via the Configuration window "Show Shadow" checkbox.

**Implementation:**
- `showShadow_` member in `RadarBeam` controls visibility
- Fragment shader discards fragments on sphere surface when disabled:
```glsl
if (!showShadow) {
    float distFromOrigin = length(LocalPos);
    float surfaceThreshold = sphereRadius * 0.05;
    if (distFromOrigin >= sphereRadius - surfaceThreshold) {
        discard;  // Fragment is on sphere surface (cap)
    }
}
```

**Files Involved:**
- `RadarBeam/RadarBeam.cpp` - showShadow uniform and shader logic
- `RadarBeam/SincBeam.cpp` - Same shader logic for Sinc beams
- `Config/SceneConfig.h` - Persistence field
- `ConfigurationWindow.cpp` - Show Shadow checkbox
- `RadarSim.cpp` - Settings read/apply, signal connections

### RCS Ray Tracing Future Work

See [RCSCompute (GPU Ray Tracing)](#rcscompute-gpu-ray-tracing) section for implementation details. Remaining work:

1. **Calculate actual RCS values** - Use hit geometry and material properties
2. **Add UI display** - Show hit count and occlusion ratio in application
3. **Visualize ray hits** - Debug rendering of traced rays and hit points
4. **Implement diffraction effects** - For realistic radar simulation
5. **Multi-target support** - Track multiple targets with inter-target reflections

## UI Layout

### Control Group Box Pattern

Both Radar Controls and Target Controls use compact layout:
```cpp
QVBoxLayout* layout = new QVBoxLayout(groupBox);
layout->setSpacing(0);                    // Minimal vertical spacing
layout->setContentsMargins(6, 0, 6, 0);   // Compact margins
```

### 0.5 Increment Slider Pattern

All angle and position sliders use 0.5 increments with QDoubleSpinBox:
```cpp
// Slider uses doubled range (e.g., -360 to 360 for -180° to 180°)
slider->setRange(-360, 360);  // Half-degree steps
slider->setValue(0);

// Spinbox shows actual value with 0.5 step
spinBox = new QDoubleSpinBox(parent);
spinBox->setRange(-180.0, 180.0);
spinBox->setSingleStep(0.5);
spinBox->setDecimals(1);
spinBox->setValue(0.0);
```

Slot handlers convert between slider (int, half-values) and actual values:
```cpp
void RadarSim::onTargetPitchSliderChanged(int value) {
    double actualValue = value / 2.0;  // Convert half-degrees to degrees
    targetPitchSpinBox_->blockSignals(true);
    targetPitchSpinBox_->setValue(actualValue);
    targetPitchSpinBox_->blockSignals(false);
    // Update controller with actualValue
}

void RadarSim::onTargetPitchSpinBoxChanged(double value) {
    targetPitchSlider_->blockSignals(true);
    targetPitchSlider_->setValue(static_cast<int>(value * 2));  // Convert to half-degrees
    targetPitchSlider_->blockSignals(false);
    // Update controller with value
}
```

### Target Controls Slots

Target control slots must call `radarSceneView_->updateScene()` after modifying the controller:
```cpp
void RadarSim::onTargetPosXSpinBoxChanged(double value) {
    targetPosXSlider_->blockSignals(true);
    targetPosXSlider_->setValue(static_cast<int>(value * 2));
    targetPosXSlider_->blockSignals(false);

    if (auto* controller = radarSceneView_->getWireframeController()) {
        QVector3D pos = controller->getPosition();
        controller->setPosition(static_cast<float>(value), pos.y(), pos.z());
        radarSceneView_->updateScene();  // IMPORTANT: Trigger repaint
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
| **Shift+Double-Click** | Pop out window to separate resizable window |

**Implementation Details** (`CameraController.cpp`):
- Orbit uses spherical coordinates (azimuth, elevation) around focus point
- Pan moves the focus point in screen-space aligned directions
- Inertia can be enabled for smooth rotation continuation after mouse release
- Elevation clamped to ±85° to avoid gimbal lock

## Pop-Out Windows

Both the 3D Radar Scene and 2D Polar RCS Plot support pop-out functionality via **Shift+Double-Click**. This creates a separate resizable window that mirrors the original view.

**Architecture:**
The pop-out system uses FBO (Framebuffer Object) texture sharing to avoid NVIDIA driver crashes that occur when reparenting QOpenGLWidget on Windows.

```
RadarGLWidget (source)
  └── FBORenderer (renders scene to texture)
        └── textureUpdated signal
              └── TextureBlitWidget (in PopOutWindow)
                    └── Displays shared texture
                    └── Forwards mouse events to source CameraController
```

**Key Components:**
- **FBORenderer**: Wraps OpenGL FBO for offscreen rendering, dynamically resizes to match pop-out window size
- **TextureBlitWidget**: QOpenGLWidget that displays the FBO texture and forwards mouse events
- **PopOutWindow**: Container window with close handling and source widget management

**Implementation Details:**
- `Qt::AA_ShareOpenGLContexts` enabled in `main.cpp` for texture sharing between GL contexts
- FBO resizes dynamically to match pop-out window dimensions for sharp rendering
- Projection matrix uses FBO dimensions (not source widget) for correct aspect ratio
- Shift+Double-Click in pop-out window closes it and returns to docked view

**Files:**
| File | Purpose |
|------|---------|
| `main.cpp` | Sets `Qt::AA_ShareOpenGLContexts` attribute |
| `PopOutWindow.h/.cpp` | Pop-out window container |
| `RadarSceneWidget/FBORenderer.h/.cpp` | FBO wrapper for offscreen rendering |
| `RadarSceneWidget/TextureBlitWidget.h/.cpp` | Displays FBO texture, forwards mouse events |
| `RadarGLWidget.cpp` | `setRenderToFBO()`, `requestFBOResize()` methods |
| `RadarSim.cpp` | `onScenePopoutRequested()`, `onPolarPopoutRequested()` slots |

## Slider Controls

All UI sliders support **left double-click to reset to default value**. This is implemented via Qt event filter in `RadarSim::eventFilter()`.

Default values are defined in `Constants.h` under `RS::Constants::Defaults` namespace:
- Sphere Radius: 100, Radar Theta: 45°, Radar Phi: 45°
- Target Position: (0, 0, 0), Rotation: (0, 0, 0), Scale: 20
- RCS Plane Offset: 0°, Thickness: 10° (index 32)

## Floating Windows

The application uses two floating windows positioned on either side of the main window:
- **Controls Window** (LEFT): Radar, target, and RCS plane parameter controls
- **Configuration Window** (RIGHT): Display options, beam settings, visualization toggles

Both windows are shown at startup and can be reopened via View menu if closed.

**Startup Layout:**
```
[Controls Window]  [Main Window (3D Scene + Polar Plot)]  [Configuration Window]
      LEFT                      CENTER                          RIGHT
```

### Controls Window (ControlsWindow)

Contains all parameter sliders and spinboxes for radar/target/RCS control.

**Window Layout:**
```
Controls Window (floating, LEFT of main)
├── Radar Controls (GroupBox)
│   ├── Sphere Radius: [Slider] [SpinBox]
│   ├── Radar Azimuth (θ): [Slider] [SpinBox]
│   └── Radar Elevation (φ): [Slider] [SpinBox]
├── Target Controls (GroupBox)
│   ├── Position X/Y/Z: [Slider] [SpinBox] each
│   ├── Pitch/Yaw/Roll: [Slider] [SpinBox] each
│   └── Scale: [Slider] [SpinBox]
└── 2D RCS Plane (GroupBox)
    ├── Cut Type: [Combo: Azimuth/Elevation]
    ├── Plane Offset: [Slider] [SpinBox]
    ├── Slice Thickness: [Slider] [SpinBox]
    └── ☑ Show Plane Fill
```

**Implementation:**
- `ControlsWindow` is a `QWidget` with `Qt::Window` flag (fixed width 270px)
- Controls panel widget created by `RadarSim::setupControlsPanel()` and transferred to window
- Positioned to LEFT of main window in `RadarSim::positionControlsWindow()`
- All sliders support double-click to reset to default (via `RadarSim::eventFilter()`)
- Menu bar: View > Controls Window reopens if closed

### Configuration Window (ConfigurationWindow)

Contains display toggles, beam type selection, and visualization options.

**Window Layout:**
```
Configuration Window (floating, RIGHT of main)
├── Configuration Profiles (GroupBox)
│   ├── Profile: [Combo]
│   └── [Save] [Save As...] [Delete] [Reset to Defaults]
├── Display Options (GroupBox)
│   ├── ☑ Show Axes
│   ├── ☑ Show Sphere
│   ├── ☑ Show Grid Lines
│   └── ☐ Enable Inertia
├── Beam Settings (GroupBox)
│   ├── ☑ Show Beam
│   ├── ☑ Show Shadow
│   └── Type: [Combo: Conical/Sinc/Phased Array]
├── Visualization (GroupBox)
│   ├── ☐ Show Reflection Lobes
│   └── ☐ Show RCS Heat Map
└── Target (GroupBox)
    ├── ☑ Show Target
    └── Type: [Combo: Cube/Cylinder/Aircraft/Sphere]
```

**Implementation:**
- `ConfigurationWindow` is a `QWidget` with `Qt::Window` flag
- Signals emitted for each setting change, connected to `RadarSim` slots
- `syncStateFromScene()` method syncs checkboxes with current scene state
- Positioned to RIGHT of main window in `RadarSim::positionConfigWindow()`
- Menu bar: View > Configuration Window reopens if closed

**Floating Window Files:**
| File | Purpose |
|------|---------|
| `ControlsWindow.h/.cpp` | Controls window container (fixed width, hide-on-close) |
| `ConfigurationWindow.h/.cpp` | Configuration window UI and signals |
| `RadarSim.cpp` | Window creation, positioning, signal connections |
| `RadarSim.h` | `controlsWindow_`, `configWindow_` members |

## Coordinate System

- **World Coordinates**: Z-up (mathematical convention)
- Spherical coordinates: (radius, theta, phi)
  - `theta`: azimuth angle (horizontal rotation)
  - `phi`: elevation angle (vertical rotation)
- Conversion to Cartesian in `RadarSiteRenderer::sphericalToCartesian()` and `RadarGLWidget::sphericalToCartesian()`

## Constants.h Organization

All compile-time constants are centralized in `Constants.h` under the `RadarSim::Constants` namespace:

```cpp
namespace RadarSim::Constants {
    // Mathematical Constants
    kPi, kPiF, kTwoPi, kTwoPiF,
    kDegToRad, kDegToRadF, kRadToDeg, kRadToDegF

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

    // Shader Visual Constants (base)
    kFresnelMin, kFresnelMax, kMinBeamOpacity,
    kAmbientStrength, kSpecularStrength, kShininess

    // Beam-Type-Specific Visibility Constants
    // Conical: kConicalFresnelBase, kConicalFresnelRange, kConicalRimLow/High/Strength, kConicalAlphaMin/Max
    // Phased: kPhasedFresnelBase, kPhasedFresnelRange, kPhasedRimLow/High/Strength, kPhasedAlphaMin/Max
    // Sinc: kSincFresnelBase, kSincFresnelRange, kSincRimLow/High/Strength, kSincAlphaMin/Max

    // Shadow Map
    kShadowMapWidth, kShadowMapHeight, kShadowMapNoHit

    // Reflection Lobe Visualization
    kMaxReflectionLobes, kLobeClusterAngle, kLobeClusterDist,
    kLobeConeLength, kLobeConeRadius, kLobeConeSegments,
    kLobeMinIntensity, kLobeBRDFDiffuse, kLobeBRDFSpecular, kLobeBRDFShininess,
    kLobeScaleLengthMin, kLobeScaleRadiusMin, kLobeColorThreshold

    // Heat Map Visualization
    kHeatMapLatBins, kHeatMapLonBins, kHeatMapMinIntensity,
    kHeatMapOpacity, kHeatMapRadiusOffset, kHeatMapLatSegments, kHeatMapLonSegments

    // Geometry Constants
    kGimbalLockThreshold

    // Sinc Beam Pattern Configuration
    kSincBeamNumSideLobes, kSincSideLobeMultiplier, kSincBeamRadialSegments

    namespace Defaults {
        kCameraDistance, kCameraAzimuth, kCameraElevation,
        kSphereRadius, kRadarTheta, kRadarPhi,
        kBeamWidth, kBeamOpacity,
        kTargetPositionX, kTargetPositionY, kTargetPositionZ,
        kTargetPitch, kTargetYaw, kTargetRoll, kTargetScale,
        kRCSPlaneOffset
    }

    namespace Colors {
        kBackgroundGrey, kGridLineGrey, kSphereOffWhite,
        kBeamOrange, kTargetGreen, kSincSideLobeColor,
        kAxisRed, kAxisGreen, kAxisBlue,
        kEquatorGreen, kPrimeMeridianBlue,
        kLobeHighIntensity, kLobeMidIntensity, kLobeLowIntensity
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
