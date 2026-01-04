# Shadow System

This document covers the GPU ray-traced shadow implementation.

## GPU Ray-Traced Shadow (Primary)

The beam uses GPU ray-traced shadows from the RCSCompute module. This provides accurate shadow visualization that matches the RCS calculation since both use the same rays.

### How It Works

1. RCSCompute generates 10,000 rays in a cone pattern from radar toward target
2. BVH traversal finds ray-triangle intersections
3. Shadow map texture (64x157) stores hit distances for each ray
4. Beam fragment shader samples shadow map and discards fragments behind hit points

### Shadow Map Format

- Dimensions: 64 (azimuth) × 157 (elevation rings)
- Value: Hit distance (positive = hit at distance, -1.0 = miss/no shadow)
- Fragment shader compares its distance to hit distance to determine visibility

### Key Implementation Details

**Compute Shader (RCSCompute::dispatchShadowMapGeneration):**
```cpp
// Store hit distance in shadow map
float hitDistance = hit.hitPoint.w;  // -1 for miss, positive for hit
imageStore(shadowMap, texCoord, vec4(hitDistance, 0.0, 0.0, 1.0));

// IMPORTANT: Unbind image after compute to allow texture sampling
glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
```

**Fragment Shader (RadarBeam):**
```glsl
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

### Files Involved

| File | Shadow-Related Code |
|------|---------------------|
| `RCSCompute/RCSCompute.cpp` | Shadow map texture, compute shader, `dispatchShadowMapGeneration()` |
| `RCSCompute/RCSCompute.h` | `getShadowMapTexture()`, `hasShadowMap()`, `shadowMapReady_` flag |
| `RadarBeam/RadarBeam.cpp` | Fragment shader UV calculation, distance comparison |
| `RadarBeam/BeamController.cpp` | Pass-through methods for shadow map parameters |
| `RadarSceneWidget/RadarGLWidget.cpp` | Connects RCSCompute shadow map to BeamController |

### Critical Notes

- Must unbind image texture after compute shader writes, before fragment shader samples
- Shadow map initialized to -1.0 (no hit) to show beam before first compute
- `shadowMapReady_` flag prevents using shadow before first compute completes
- Uses `LocalPos` (untransformed vertex position) for UV calculation to match ray coordinates

## Show Shadow Toggle

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

## RCS Ray Tracing Future Work

1. **Calculate actual RCS values** - Use hit geometry and material properties
2. **Add UI display** - Show hit count and occlusion ratio in application
3. **Visualize ray hits** - Debug rendering of traced rays and hit points
4. **Implement diffraction effects** - For realistic radar simulation
5. **Multi-target support** - Track multiple targets with inter-target reflections

## Beam Transparency Through Shadow (Nice to Have)

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
