# Adding Features to RadarSim

This guide shows how to add new beam types, target types, UI controls, and components.

## Adding a UI Control

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

## Adding a New Beam Type

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

## Adding a New Solid Target Type

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

## Adding a Component

1. Create header/source in appropriate directory
2. Add to `CMakeLists.txt` source lists
3. Include in `RadarSceneWidget::createComponents()`
4. Initialize in `RadarGLWidget::initializeGL()`
5. Render in `RadarGLWidget::paintGL()`

## Beam Hierarchy Reference

```
RadarBeam (base)
├── ConicalBeam      # Uniform intensity cone
├── PhasedArrayBeam  # Main lobe + side lobes (phased array pattern)
├── SincBeam         # Sinc² intensity pattern with natural side lobes
└── SingleRayBeam    # Diagnostic single ray for bounce visualization
```

**BeamType enum:** `Conical`, `Shaped`, `Phased`, `Sinc`, `SingleRay`

**SingleRayBeam Details:**
- Traces a single ray from radar toward target center
- Used with BounceRenderer to visualize multi-bounce ray paths
- Supports Path mode (geometric bounces) and Physics mode (reflections)
- Enable "Show Bounces" in Configuration window to see ray path

**SincBeam Details:**
- Intensity follows sinc²(πθ/θmax) pattern (realistic electromagnetic field)
- 7-float vertex format: `[x, y, z, nx, ny, nz, intensity]`
- Extended geometry (4× main lobe via `kSincSideLobeMultiplier`) for visible side lobes
- Per-vertex intensity drives color blending and alpha modulation
- `getVisualExtentMultiplier()` returns 4.0 for proper shadow ray coverage

Factory method for beam creation:
```cpp
RadarBeam* RadarBeam::createBeam(BeamType type, float sphereRadius, float beamWidthDegrees);
```

## WireframeTarget Hierarchy Reference

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

Factory method:
```cpp
std::unique_ptr<WireframeTarget> WireframeTarget::createTarget(WireframeType type);
```

WireframeTarget uses solid surface rendering (GL_TRIANGLES) with indexed drawing (EBO). Vertex format is `[x, y, z, nx, ny, nz]` where the normal vector supports diffuse + ambient lighting. Face culling (`GL_CULL_FACE`) ensures only outside surfaces are drawn.

**Target Rendering Features:**
- **Radar angle-based edge shading**: Faces perpendicular to radar direction appear darker, enhancing silhouette visibility. Uses `smoothstep(0.0, 0.4, radarDot)` in fragment shader.
- **Crease edge detection**: Only structural edges rendered (where adjacent face normals differ >10°), eliminating internal tessellation lines.
