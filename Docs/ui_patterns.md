# UI Patterns

This document covers Qt UI patterns used in RadarSim.

## Control Group Box Pattern

Both Radar Controls and Target Controls use compact layout:
```cpp
QVBoxLayout* layout = new QVBoxLayout(groupBox);
layout->setSpacing(0);                    // Minimal vertical spacing
layout->setContentsMargins(6, 0, 6, 0);   // Compact margins
```

## 0.5 Increment Slider Pattern

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

## Target Controls Slots

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

## Slider Double-Click Reset

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

### Controls Window Layout

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

### Configuration Window Layout

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
│   ├── ☐ Show Bounces
│   ├── Type: [Combo: Conical/Sinc/Phased Array/SingleRay]
│   └── Ray Trace Mode: [Radio: Path | Physics]
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

## Control Widget Pattern

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

## Floating Window Files

| File | Purpose |
|------|---------|
| `ControlsWindow.h/.cpp` | Controls window container (fixed width, hide-on-close) |
| `ConfigurationWindow.h/.cpp` | Configuration window UI and signals |
| `RadarSim.cpp` | Window creation, positioning, signal connections |
| `RadarSim.h` | `controlsWindow_`, `configWindow_` members |
