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

    void loadFromJson(const QJsonObject& obj) {
        sphereRadius = static_cast<float>(obj.value("sphereRadius").toDouble(sphereRadius));
        radarTheta = static_cast<float>(obj.value("radarTheta").toDouble(radarTheta));
        radarPhi = static_cast<float>(obj.value("radarPhi").toDouble(radarPhi));
        showSphere = obj.value("showSphere").toBool(showSphere);
        showGrid = obj.value("showGrid").toBool(showGrid);
        showAxes = obj.value("showAxes").toBool(showAxes);
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["sphereRadius"] = static_cast<double>(sphereRadius);
        obj["radarTheta"] = static_cast<double>(radarTheta);
        obj["radarPhi"] = static_cast<double>(radarPhi);
        obj["showSphere"] = showSphere;
        obj["showGrid"] = showGrid;
        obj["showAxes"] = showAxes;
        return obj;
    }
};

} // namespace RSConfig
