# RadarSim TODO List

**Last Updated:** December 2024

## Recently Completed

- [x] Solid surface rendering for WireframeTarget (GL_TRIANGLES with EBO)
- [x] Cube, Cylinder, and Aircraft solid target shapes
- [x] Diffuse + ambient lighting for targets
- [x] Face culling for proper solid rendering
- [x] Target Controls UI with sliders for Position (X/Y/Z), Rotation (Pitch/Yaw/Roll), Scale
- [x] Target Controls connected to WireframeTargetController (scene updates on slider change)
- [x] Compact UI layout for Radar Controls and Target Controls (setSpacing(2), setContentsMargins(6,6,6,6))
- [x] Context menu for target type selection (Cube, Cylinder, Aircraft)
- [x] Beam type switching via context menu (Conical, Phased Array)

## In Progress / Known Issues

### Shadow Volume (Deferred)

The shadow volume system for beam occlusion behind targets has issues:

- [ ] **Shadow does NOT follow scene rotations** - When the scene is rotated via mouse drag, the shadow doesn't track correctly
- [ ] **Depth cap disabled** - The Z-fail algorithm depth cap causes target/beam to become invisible; currently disabled
- [ ] **Far hemisphere issue** - Shadow doesn't work when radar is on the far side of the sphere

**Status:** Deferred for later investigation. Core functionality works for basic cases.

**Attempted fix (Dec 2024):** Changed `renderShadowVolume()` to use identity matrix instead of `sceneModel`, reasoning that shadow vertices are already in world space. This did NOT fix the issue - the problem may be deeper in the coordinate space chain (possibly in `generateShadowVolume()` or how radar position is transformed).

## Future Work

### High Priority

- [ ] Fix shadow volume scene rotation tracking
- [ ] Investigate depth cap visibility issues for proper Z-fail algorithm
- [ ] Add more target shapes (Pyramid, Sphere, Ship) - optional

### Medium Priority

- [ ] Radar Cross Section (RCS) calculations using shadow volume geometry
- [ ] Diffraction effects for realistic radar simulation
- [ ] Material properties for targets (reflectivity, absorption)

### Low Priority / Technical Debt

- [x] Remove legacy SphereWidget (keep only component-based RadarGLWidget)
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
├── SphereRenderer      - Sphere, grid, axes, radar dot
├── BeamController      - Conical/Phased beam rendering
├── CameraController    - Mouse interaction, view transforms, inertia
├── ModelManager        - 3D model loading (placeholder)
└── WireframeTargetController - Solid target shapes with transforms
```

### Build Notes

Build from VS Developer Command Prompt or use the CMake preset:
```bash
cmake --preset Qt-Debug
cmake --build out/build/debug --config Debug
```

Or use Visual Studio with CMake integration (open folder).
