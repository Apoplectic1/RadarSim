#pragma once

// RadarSim Compile-Time Constants
// These are fixed values that do not change at runtime.
// For user-configurable settings, see Config/AppSettings.h

namespace RadarSim {
namespace Constants {

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

// =============================================================================
// Shadow Map Configuration
// =============================================================================
constexpr int kShadowMapWidth = 64;             // Shadow map texture width (azimuth resolution)
constexpr int kShadowMapHeight = 157;           // Shadow map texture height (elevation rings)
constexpr float kShadowMapNoHit = -1.0f;        // Value indicating no shadow hit

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
    constexpr float kSphereOffWhite[3] = {0.95f, 0.95f, 0.95f};

    // Default object colors
    constexpr float kBeamOrange[3] = {1.0f, 0.5f, 0.0f};
    constexpr float kTargetGreen[3] = {0.0f, 1.0f, 0.0f};

    // Axis colors (RGB for XYZ)
    constexpr float kAxisRed[3] = {1.0f, 0.0f, 0.0f};
    constexpr float kAxisGreen[3] = {0.0f, 1.0f, 0.0f};
    constexpr float kAxisBlue[3] = {0.0f, 0.0f, 1.0f};

    // Grid special lines
    constexpr float kEquatorGreen[3] = {0.0f, 0.8f, 0.0f};
    constexpr float kPrimeMeridianBlue[3] = {0.0f, 0.0f, 0.8f};
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
