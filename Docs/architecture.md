# RadarSim Architecture

This document covers the threading model, GPU compute pipeline, and rendering architecture.

## Threading Model

```
┌────────────────────────────────────────────────────────────────┐
│                      MAIN THREAD                               │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐ │
│  │   Qt UI     │  │   OpenGL    │  │     GPU Compute         │ │
│  │  (widgets)  │  │  Rendering  │  │   (dispatch + wait)     │ │
│  └─────────────┘  └─────────────┘  └─────────────────────────┘ │
└────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌────────────────────────────────────────────────────────────────┐
│                         GPU                                    │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐  │
│  │ Ray Gen      │→ │ BVH Traverse │→ │ Shadow Map Gen       │  │
│  │ Compute      │  │ Compute      │  │ Compute              │  │
│  └──────────────┘  └──────────────┘  └──────────────────────┘  │
└────────────────────────────────────────────────────────────────┘
```

**Current Constraints:**
- All work happens on main thread (Qt requirement for UI and GL context)
- GPU compute dispatched synchronously - CPU blocks waiting for results
- No background threads for CPU-intensive work

## GPU Compute Pipeline (RCSCompute)

**Three-stage compute shader pipeline:**

| Stage | Shader | Input | Output | Workgroup |
|-------|--------|-------|--------|-----------|
| 1. Ray Generation | `dispatchRayGeneration()` | Radar position, beam params | Ray buffer (SSBO 0) | 64 threads |
| 2. BVH Traversal | `dispatchTracing()` | Rays, BVH, triangles | Hit results (SSBO 3) | 64 threads |
| 3. Shadow Map | `dispatchShadowMapGeneration()` | Hit results | Shadow texture (Image 0) | 64 threads |

**Synchronization between stages:**
```cpp
glDispatchCompute(numGroups, 1, 1);
glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);  // Wait for SSBO writes
```

## CPU ↔ GPU Data Flow

**Upload (CPU → GPU):**

| Data | Size | Frequency | Method |
|------|------|-----------|--------|
| BVH nodes | Variable | On geometry change | `glBufferData()` |
| Triangles | Variable | On geometry change | `glBufferData()` |
| Uniforms | ~100 bytes | Every frame | `setUniformValue()` |

**Readback (GPU → CPU):**

| Data | Size | Frequency | Method | Blocking? |
|------|------|-----------|--------|-----------|
| Hit counter | 4 bytes | Every frame | `glMapBufferRange()` | **YES** |
| Hit results | ~625 KB | When viz enabled | `glMapBufferRange()` | **YES** |
| Shadow map | 40 KB | Never | Stays on GPU | No |

## Current Bottlenecks

1. **Atomic Counter Readback** - 4-byte read causes full GPU stall every frame
2. **Hit Buffer Readback** - 640 KB synchronous transfer when visualization enabled
3. **BVH Construction** - CPU-side, blocks main thread on geometry change

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

**Render Order Notes:**
- Solid targets render before beam so they are visible through semi-transparent beam
- Target rendering explicitly sets depth test, disables blending, and enables face culling
- RCSCompute generates both RCS data and shadow map texture which BeamController uses
- ReflectionRenderer renders last with alpha blending for proper transparency

## Component Pattern

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

RCS computation happens **synchronously inside `paintGL()`** every frame:

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

When idle (no interaction, no inertia), the scene doesn't render - no wasted GPU cycles.

## Compute Architecture Files

| File | Responsibility |
|------|----------------|
| `RCSCompute.cpp` | GPU compute dispatch, buffer management |
| `BVHBuilder.cpp` | CPU-side BVH construction (candidate for threading) |
| `RadarGLWidget.cpp` | Orchestrates compute + render in `paintGL()` |
