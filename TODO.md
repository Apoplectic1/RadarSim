# RadarSim TODO List

**Last Updated:** December 2024

## Recently Completed

- [x] **2D Polar RCS Plot** - Real-time polar plot showing RCS vs angle
  - OpenGL-based PolarRCSPlot widget below 3D scene
  - AzimuthCutSampler and ElevationCutSampler for slicing RCS data
  - Slicing plane visualization in 3D scene (translucent rectangle)
  - UI controls: Cut Type, Plane Offset, Slice Thickness, Show Fill
  - Non-linear thickness slider (0.1° steps for 0.5-3°, 1° steps for 3-40°)
- [x] **Geodesic Sphere Target** - For RCS verification against theoretical πr²
  - Icosahedron subdivision (configurable levels, default 3 = 1280 faces)
  - Added to context menu and target type enum
- [x] **0.5° Slider Increments** - Finer control for all angle/position sliders
  - Radar Azimuth/Elevation use QDoubleSpinBox with 0.5° step
  - Target Position X/Y/Z use 0.5 unit increments
  - Target Pitch/Yaw/Roll use 0.5° increments
  - Target Scale uses 0.5 unit increments
- [x] **Profile Save/Restore** - New settings persisted to profiles
  - Target type (Cube/Cylinder/Aircraft/Sphere) saved and restored
  - RCS plane settings (cut type, offset, thickness, show fill) saved
  - Context menu checkmarks sync with restored settings
- [x] **HeatMapRenderer** - RCS heat map visualization on radar sphere surface
  - Smooth gradient (Blue→Yellow→Red) based on reflection intensity
  - Spherical binning (64×64 lat/lon) for intensity accumulation
  - Per-vertex interpolation for smooth appearance
  - Toggle via "Toggle RCS Heat Map" context menu
- [x] **SincBeam** - Realistic sinc² intensity pattern with visible side lobes
- [x] **Show Shadow toggle** - Control beam footprint visibility on sphere
- [x] **GPU ray-traced shadows** - Accurate beam occlusion from RCS ray tracing
- [x] **ReflectionRenderer** - Colored cone lobes showing reflection directions
- [x] Solid surface rendering for WireframeTarget (GL_TRIANGLES with EBO)
- [x] Cube, Cylinder, and Aircraft solid target shapes
- [x] Diffuse + ambient lighting for targets
- [x] Face culling for proper solid rendering
- [x] Target Controls UI with sliders for Position (X/Y/Z), Rotation (Pitch/Yaw/Roll), Scale
- [x] Target Controls connected to WireframeTargetController (scene updates on slider change)
- [x] Compact UI layout for Radar Controls and Target Controls (setSpacing(2), setContentsMargins(6,6,6,6))
- [x] Context menu for target type selection (Cube, Cylinder, Aircraft, Sphere)
- [x] Beam type switching via context menu (Conical, Sinc, Phased Array)

## In Progress / Known Issues

No critical issues at present. GPU ray-traced shadows replaced the problematic shadow volume approach.

## Future Work

### High Priority

- [ ] Calculate actual RCS values from hit geometry (currently just visualization)
- [ ] Add RCS value display in UI (dBsm readout from polar plot data)
- [ ] Add more target shapes (Pyramid, Ship, Custom OBJ import)
- [ ] Implement ModelManager intersection testing (currently placeholder - impacts accuracy)
- [ ] RCS Validation with Sphere target (compare calculated RCS against theoretical πr²)

### Medium Priority

- [ ] Diffraction effects for realistic radar simulation (significant RCS impact for certain geometries)
- [ ] Material properties for targets (reflectivity, absorption)
- [ ] Pop-out windows for 3D scene and polar plot (resizable analysis windows)
- [ ] Multi-target support with inter-target reflections

### Low Priority / Technical Debt

- [x] Remove legacy SphereWidget (keep only component-based RadarGLWidget)
- [x] Consolidate magic numbers into Constants.h (Colors, Lighting, View, UI namespaces)
- [ ] Unify memory management (raw pointers vs std::shared_ptr)
- [ ] Add unit test coverage
- [x] Remove excessive qDebug() logging

## Important Enhancements

### Beam Steering / Scanning

- [ ] Add controls for azimuth and elevation angles of the beam
- [ ] Implement basic scanning patterns (sector scan, raster scan)
- [ ] Animate beam scanning with configurable rate

### Range Resolution

- [ ] Define and implement range resolution limit
- [ ] Handle closely spaced reflecting surfaces appropriately

### Performance Optimization

- [ ] Profile application to identify performance bottlenecks
- [ ] Optimize ray tracing for complex target geometries
- [ ] Consider LOD (Level of Detail) for distant targets

### Radar Configuration

- [ ] Allow radar settings to be configured (frequency, pulse width, PRF)
- [ ] Save/load radar configuration profiles
- [ ] Display radar parameters in UI

### Documentation

- [ ] Expand architecture documentation
- [ ] Document RCS calculation methods and algorithms
- [ ] Document public API for each component
- [ ] Add inline code comments for complex algorithms

### Testing & Validation

- [ ] Define test scenarios for RCS accuracy verification
- [ ] Create reference test cases with known analytical solutions
- [ ] Implement automated regression tests
- [ ] Add error handling and logging throughout application

## UI Improvements

- [x] Tightened vertical spacing in control group boxes
- [x] Main window minimum size adjusted (1024x900)
- [ ] Target color picker
- [ ] Beam color/opacity controls in UI (currently only via code)
- [ ] RCS results panel with numerical readouts
- [ ] Radar configuration panel

## Architecture Notes

### Current Component Structure

```
RadarGLWidget
├── SphereRenderer           - Sphere, grid, axes, radar dot
├── BeamController           - Conical/Sinc/Phased beam rendering
├── CameraController         - Mouse interaction, view transforms, inertia
├── ModelManager             - 3D model loading (placeholder)
├── WireframeTargetController- Solid target shapes (Cube/Cylinder/Aircraft/Sphere)
├── RCSCompute               - GPU ray tracing for RCS calculations
│   ├── AzimuthCutSampler    - Sample RCS data for horizontal plane cuts
│   ├── ElevationCutSampler  - Sample RCS data for vertical plane cuts
│   └── SlicingPlaneRenderer - Visualize sampling plane in 3D
├── ReflectionRenderer       - Colored cone lobes (RCS visualization)
├── HeatMapRenderer          - Sphere heat map (RCS visualization)
└── PolarRCSPlot             - 2D polar plot widget (separate from GL scene)
```

### Build Notes

Build from VS Developer Command Prompt or use the CMake preset:
```bash
cmake --preset Qt-Debug
cmake --build out/build/debug --config Debug
```

Or use Visual Studio with CMake integration (open folder).
