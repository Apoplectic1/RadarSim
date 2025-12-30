# RadarSim TODO List

**Last Updated:** December 2024

## Recently Completed

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
- [x] Context menu for target type selection (Cube, Cylinder, Aircraft)
- [x] Beam type switching via context menu (Conical, Sinc, Phased Array)

## In Progress / Known Issues

No critical issues at present. GPU ray-traced shadows replaced the problematic shadow volume approach.

## Future Work

### High Priority

- [ ] Calculate actual RCS values from hit geometry (currently just visualization)
- [ ] Add RCS value display in UI (dBsm readout)
- [ ] Add more target shapes (Pyramid, Sphere, Ship)

### Medium Priority

- [ ] Diffraction effects for realistic radar simulation
- [ ] Material properties for targets (reflectivity, absorption)
- [ ] Heat map persistence/settings save

### Low Priority / Technical Debt

- [x] Remove legacy SphereWidget (keep only component-based RadarGLWidget)
- [x] Consolidate magic numbers into Constants.h (Colors, Lighting, View, UI namespaces)
- [ ] Unify memory management (raw pointers vs std::shared_ptr)
- [ ] Implement ModelManager intersection testing (currently placeholder)
- [ ] Add unit test coverage
- [x] Remove excessive qDebug() logging

## UI Improvements

- [x] Tightened vertical spacing in control group boxes
- [x] Main window minimum size adjusted (1024x900)
- [ ] Consider adding target color picker
- [ ] Consider adding beam color/opacity controls to UI (currently only via code)

## Architecture Notes

### Current Component Structure

```
RadarGLWidget
├── SphereRenderer           - Sphere, grid, axes, radar dot
├── BeamController           - Conical/Sinc/Phased beam rendering
├── CameraController         - Mouse interaction, view transforms, inertia
├── ModelManager             - 3D model loading (placeholder)
├── WireframeTargetController- Solid target shapes with transforms
├── RCSCompute               - GPU ray tracing for RCS calculations
├── ReflectionRenderer       - Colored cone lobes (RCS visualization)
└── HeatMapRenderer          - Sphere heat map (RCS visualization)
```

### Build Notes

Build from VS Developer Command Prompt or use the CMake preset:
```bash
cmake --preset Qt-Debug
cmake --build out/build/debug --config Debug
```

Or use Visual Studio with CMake integration (open folder).
