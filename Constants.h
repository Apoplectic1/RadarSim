#pragma once

// RadarSim Compile-Time Constants
// These are fixed values that do not change at runtime.
// For user-configurable settings, see Config/AppSettings.h

namespace RadarSim {
namespace Constants {

// =============================================================================
// Mathematical Constants
// =============================================================================
constexpr double kPi = 3.14159265358979323846;
constexpr float kPiF = 3.14159265358979323846f;  // Float version for GPU/OpenGL code
constexpr double kTwoPi = 2.0 * kPi;
constexpr float kTwoPiF = 2.0f * kPiF;
constexpr double kDegToRad = kPi / 180.0;
constexpr float kDegToRadF = kPiF / 180.0f;
constexpr double kRadToDeg = 180.0 / kPi;
constexpr float kRadToDegF = 180.0f / kPiF;

// =============================================================================
// Compute Shader Configuration
// =============================================================================
constexpr int kComputeWorkgroupSize = 64;       // GPU workgroup size for compute shaders
constexpr int kDefaultNumRays = 10000;          // Default number of rays for RCS computation
constexpr int kRaysPerRing = 64;                // Rays per elevation ring in cone pattern

// =============================================================================
// BVH (Bounding Volume Hierarchy) Settings
// =============================================================================
constexpr int kBVHMaxLeafSize = 4;              // Maximum triangles per leaf node
constexpr int kBVHNumBins = 12;                 // Number of bins for SAH split finding
constexpr int kBVHStackSize = 62;               // Stack size for iterative BVH traversal

// =============================================================================
// Geometry Generation - Segment/Resolution Counts
// =============================================================================
constexpr int kBeamConeSegments = 32;           // Segments around beam cone circumference
constexpr int kBeamCapRings = 8;                // Rings for beam cap fill
constexpr int kCylinderSegments = 24;           // Segments around cylinder circumference
constexpr int kSphereLatSegments = 72;          // Latitude segments for sphere mesh
constexpr int kSphereLongSegments = 50;         // Longitude segments for sphere mesh
constexpr int kAxisArrowSegments = 12;          // Segments for axis arrow cones

// =============================================================================
// Camera Limits and Behavior
// =============================================================================
constexpr float kCameraMinDistance = 50.0f;     // Minimum zoom distance
constexpr float kCameraMaxDistance = 1000.0f;   // Maximum zoom distance
constexpr float kCameraMaxElevation = 1.5f;     // Max elevation angle (~85 degrees)
constexpr float kCameraRotationSpeed = 0.005f;  // Mouse rotation sensitivity
constexpr float kCameraZoomSpeed = 0.5f;        // Mouse wheel zoom sensitivity
constexpr float kCameraInertiaDecay = 0.95f;    // Inertia velocity decay factor
constexpr int kCameraInertiaTimerMs = 16;       // Inertia timer interval (~60 FPS)
constexpr float kCameraInertiaScaleFactor = 0.3f; // Inertia velocity scale factor
constexpr float kCameraVelocityThreshold = 0.001f; // Min velocity to trigger inertia

// =============================================================================
// Ray Tracing Precision
// =============================================================================
constexpr float kRayTMinEpsilon = 0.001f;       // Minimum ray distance to avoid self-intersection
constexpr float kTriangleEpsilon = 1e-8f;       // Epsilon for triangle intersection tests
constexpr float kMaxRayDistanceMultiplier = 3.0f; // Max ray distance as multiple of scene size

// =============================================================================
// Rendering Parameters
// =============================================================================
constexpr float kGridLineWidthNormal = 1.0f;    // Standard grid line width
constexpr float kGridLineWidthSpecial = 3.5f;   // Major grid line width (equator, prime meridian)
constexpr float kGridRadiusOffset = 1.005f;     // Grid radius multiplier (slightly outside sphere)
constexpr float kRadarDotRadius = 5.0f;         // Radar position dot size
constexpr int kRadarDotVertices = 16;           // Vertices in radar dot circle

// =============================================================================
// Shader Visual Constants
// =============================================================================
constexpr float kFresnelMin = 0.3f;             // Minimum fresnel contribution
constexpr float kFresnelMax = 0.7f;             // Maximum fresnel contribution
constexpr float kMinBeamOpacity = 0.1f;         // Minimum beam alpha (prevents full transparency)
constexpr float kAmbientStrength = 0.3f;        // Ambient lighting strength
constexpr float kSpecularStrength = 0.5f;       // Specular highlight strength
constexpr float kShininess = 32.0f;             // Specular shininess exponent
constexpr float kNormalBlendFactor = 0.25f;     // Normal blending factor for phased array beam

// Conical Beam Visibility Constants
// FresnelBase: opacity when viewing beam head-on (0=transparent, 1=opaque)
// FresnelRange: how much more opaque edges become vs center (higher=more edge glow)
// RimLow/RimHigh: view angle range where rim glow appears (0=head-on, 1=edge-on)
// RimStrength: intensity of the bright edge outline (0=none, 1=very bright)
// AlphaMin/Max: hard limits on final transparency (raise AlphaMin to prevent invisible areas)
constexpr float kConicalFresnelBase = 0.51f;     // Base opacity at center
constexpr float kConicalFresnelRange = 0.2f;    // Additional opacity at edges
constexpr float kConicalRimLow = 0.6f;          // Rim starts appearing at this angle
constexpr float kConicalRimHigh = 0.95f;        // Rim at full strength at this angle
constexpr float kConicalRimStrength = 0.1f;     // Rim glow brightness
constexpr float kConicalAlphaMin = 0.03f;       // Floor - beam never more transparent than this
constexpr float kConicalAlphaMax = 0.6f;        // Ceiling - beam never more opaque than this

// Phased Array Beam Visibility Constants
// (Same meaning as Conical - see comments above)
constexpr float kPhasedFresnelBase = 0.1f;      // Base opacity at center
constexpr float kPhasedFresnelRange = 0.2f;     // Additional opacity at edges
constexpr float kPhasedRimLow = 0.6f;           // Rim starts appearing at this angle
constexpr float kPhasedRimHigh = 0.95f;         // Rim at full strength at this angle
constexpr float kPhasedRimStrength = 0.1f;      // Rim glow brightness
constexpr float kPhasedAlphaMin = 0.03f;        // Floor - beam never more transparent than this
constexpr float kPhasedAlphaMax = 0.6f;         // Ceiling - beam never more opaque than this

// SincBeam Visibility Constants
// Sinc beam has per-vertex intensity from the sinc² pattern, so it has extra controls.
// FresnelBase/Range and Rim* work the same as Conical (see comments above).
// IntensityAlphaMin: even low-intensity side lobes stay at least this visible (0-1)
// OpacityMult: global multiplier applied after all other calculations (>1 = brighter)
constexpr float kSincFresnelBase = 0.6f;        // Base opacity at center
constexpr float kSincFresnelRange = 0.4f;       // Additional opacity at edges
constexpr float kSincRimLow = 0.3f;             // Rim starts appearing at this angle
constexpr float kSincRimHigh = 0.7f;            // Rim at full strength at this angle
constexpr float kSincRimStrength = 0.4f;        // Rim glow brightness
constexpr float kSincIntensityAlphaMin = 0.5f;  // Side lobes stay at least this visible
constexpr float kSincOpacityMult = 1.5f;        // Final multiplier (>1 = more visible overall)
constexpr float kSincAlphaMin = 0.3f;           // Floor - beam never more transparent than this
constexpr float kSincAlphaMax = 1.0f;           // Ceiling - beam never more opaque than this

// =============================================================================
// Shadow Map Configuration
// =============================================================================
constexpr int kShadowMapWidth = 64;             // Shadow map texture width (azimuth resolution)
constexpr int kShadowMapHeight = 157;           // Shadow map texture height (elevation rings)
constexpr float kShadowMapNoHit = -1.0f;        // Value indicating no shadow hit

// =============================================================================
// Reflection Lobe Visualization
// =============================================================================
constexpr int kMaxReflectionLobes = 100;        // Maximum aggregated lobe clusters
constexpr int kMaxTargets = 16;                 // Maximum simultaneous targets
constexpr float kLobeClusterAngle = 10.0f;      // Clustering angle threshold (degrees)
constexpr float kLobeClusterDist = 5.0f;        // Clustering spatial distance threshold
constexpr float kLobeConeLength = 15.0f;        // Length of visualization cones
constexpr float kLobeConeRadius = 3.0f;         // Base radius of visualization cones
constexpr int kLobeConeSegments = 12;           // Segments around cone circumference
constexpr float kLobeMinIntensity = 0.05f;      // Minimum intensity to display lobe
constexpr float kLobeBRDFDiffuse = 0.3f;        // BRDF diffuse coefficient
constexpr float kLobeBRDFSpecular = 0.7f;       // BRDF specular coefficient
constexpr float kLobeBRDFShininess = 32.0f;     // BRDF specular exponent
constexpr float kLobeScaleLengthMin = 0.5f;     // Min length scale at zero intensity
constexpr float kLobeScaleRadiusMin = 0.3f;     // Min radius scale at zero intensity
constexpr float kLobeColorThreshold = 0.5f;     // Intensity threshold for color interpolation

// =============================================================================
// Heat Map Visualization
// =============================================================================
constexpr int kHeatMapLatBins = 1024;             // Latitude bins for spherical accumulation
constexpr int kHeatMapLonBins = 1024;             // Longitude bins for spherical accumulation
constexpr float kHeatMapMinIntensity = 0.05f;   // Minimum intensity threshold to display
constexpr float kHeatMapOpacity = 0.7f;         // Default heat map opacity
constexpr float kHeatMapRadiusOffset = 1.02f;   // Render slightly above sphere surface
constexpr int kHeatMapLatSegments = 64;         // Sphere mesh latitude segments
constexpr int kHeatMapLonSegments = 64;         // Sphere mesh longitude segments

// =============================================================================
// Geometry Constants
// =============================================================================
constexpr float kGimbalLockThreshold = 0.99f;   // Dot product threshold for gimbal lock avoidance

// =============================================================================
// Sinc Beam Pattern Configuration
// =============================================================================
constexpr int kSincBeamNumSideLobes = 3;        // Number of side lobes to render
constexpr float kSincSideLobeMultiplier = 4.0f; // Geometry extends 4x main lobe for side lobes + edge fade
constexpr int kSincBeamRadialSegments = 64;     // Radial resolution for intensity gradient

// =============================================================================
// Default Values (used when no config loaded)
// =============================================================================
namespace Defaults {
    // Camera
    constexpr float kCameraDistance = 300.0f;
    constexpr float kCameraAzimuth = 0.0f;
    constexpr float kCameraElevation = 0.4f;

    // Sphere/Scene
    constexpr float kSphereRadius = 100.0f;
    constexpr float kRadarTheta = 45.0f;
    constexpr float kRadarPhi = 45.0f;

    // Beam
    constexpr float kBeamWidth = 15.0f;         // Beam width in degrees
    constexpr float kBeamOpacity = 0.3f;

    // Target
    constexpr float kTargetScale = 20.0f;
}

// =============================================================================
// Colors (RGB float arrays)
// =============================================================================
namespace Colors {
    // Scene colors
    constexpr float kBackgroundGrey[3] = {0.5f, 0.5f, 0.5f};
    constexpr float kGridLineGrey[3] = {0.4f, 0.4f, 0.4f};
    constexpr float kSphereOffWhite[3] = { 0.95f, 0.95f, 0.95f };

    // Default object colors
    constexpr float kBeamOrange[3] = {1.0f, 0.5f, 0.0f};
    constexpr float kTargetGreen[3] = {0.0f, 1.0f, 0.0f};
    constexpr float kSincSideLobeColor[3] = {0.3f, 0.15f, 0.0f};  // Darker orange for side lobes

    // Axis colors (RGB for XYZ)
    constexpr float kAxisRed[3] = {1.0f, 0.0f, 0.0f};
    constexpr float kAxisGreen[3] = {0.0f, 1.0f, 0.0f};
    constexpr float kAxisBlue[3] = {0.0f, 0.0f, 1.0f};

    // Grid special lines
    constexpr float kEquatorGreen[3] = {0.0f, 0.8f, 0.0f};
    constexpr float kPrimeMeridianBlue[3] = {0.0f, 0.0f, 0.8f};

    // Reflection lobe intensity gradient (high to low)
    constexpr float kLobeHighIntensity[3] = {1.0f, 0.0f, 0.0f};   // Red (high energy)
    constexpr float kLobeMidIntensity[3] = {1.0f, 1.0f, 0.0f};    // Yellow (medium)
    constexpr float kLobeLowIntensity[3] = {0.0f, 0.0f, 1.0f};    // Blue (low energy)
}

// =============================================================================
// Lighting
// =============================================================================
namespace Lighting {
    constexpr float kLightPosition[3] = {500.0f, -500.0f, 500.0f};
    constexpr float kTargetLightPosition[3] = {500.0f, 500.0f, 500.0f};
}

// =============================================================================
// View/Perspective
// =============================================================================
namespace View {
    constexpr float kPerspectiveFOV = 45.0f;        // Field of view in degrees
    constexpr float kNearPlane = 0.1f;              // Near clipping plane
    constexpr float kFarPlane = 2000.0f;            // Far clipping plane
    constexpr float kAxisLengthMultiplier = 1.2f;   // Axis length as fraction of radius
}

// =============================================================================
// UI Constants
// =============================================================================
namespace UI {
    constexpr int kAxisLabelFontSize = 14;          // Font size for axis labels
    constexpr int kTextOffsetPixels = 15;           // Offset for text labels
}

}} // namespace RadarSim::Constants
