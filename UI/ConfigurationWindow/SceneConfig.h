#pragma once

#include <QJsonObject>

namespace RSConfig {

struct SceneConfig {
    float sphereRadius = 100.0f;
    float radarTheta = 45.0f;   // Azimuth angle in degrees
    float radarPhi = 45.0f;     // Elevation angle in degrees
    bool showSphere = true;
    bool showGrid = true;
    bool showAxes = true;
    bool showShadow = true;  // Show beam projection (shadow) on sphere

    // RCS slicing plane settings
    int rcsCutType = 0;          // 0 = Azimuth, 1 = Elevation
    float rcsPlaneOffset = 0.0f; // Offset angle in degrees
    float rcsSliceThickness = 10.0f; // ±degrees (updated default)
    bool rcsPlaneShowFill = true;  // Show translucent fill in slicing plane

    void loadFromJson(const QJsonObject& obj) {
        sphereRadius = static_cast<float>(obj.value("sphereRadius").toDouble(sphereRadius));
        radarTheta = static_cast<float>(obj.value("radarTheta").toDouble(radarTheta));
        radarPhi = static_cast<float>(obj.value("radarPhi").toDouble(radarPhi));
        showSphere = obj.value("showSphere").toBool(showSphere);
        showGrid = obj.value("showGrid").toBool(showGrid);
        showAxes = obj.value("showAxes").toBool(showAxes);
        showShadow = obj.value("showShadow").toBool(showShadow);

        // RCS plane settings
        rcsCutType = obj.value("rcsCutType").toInt(rcsCutType);
        rcsPlaneOffset = static_cast<float>(obj.value("rcsPlaneOffset").toDouble(rcsPlaneOffset));
        rcsSliceThickness = static_cast<float>(obj.value("rcsSliceThickness").toDouble(rcsSliceThickness));
        rcsPlaneShowFill = obj.value("rcsPlaneShowFill").toBool(rcsPlaneShowFill);
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["sphereRadius"] = static_cast<double>(sphereRadius);
        obj["radarTheta"] = static_cast<double>(radarTheta);
        obj["radarPhi"] = static_cast<double>(radarPhi);
        obj["showSphere"] = showSphere;
        obj["showGrid"] = showGrid;
        obj["showAxes"] = showAxes;
        obj["showShadow"] = showShadow;

        // RCS plane settings
        obj["rcsCutType"] = rcsCutType;
        obj["rcsPlaneOffset"] = static_cast<double>(rcsPlaneOffset);
        obj["rcsSliceThickness"] = static_cast<double>(rcsSliceThickness);
        obj["rcsPlaneShowFill"] = rcsPlaneShowFill;
        return obj;
    }
};

} // namespace RSConfig
