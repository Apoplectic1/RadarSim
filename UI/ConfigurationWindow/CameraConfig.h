#pragma once

#include <QJsonObject>
#include <QJsonArray>
#include <QVector3D>

namespace RSConfig {

struct CameraConfig {
    float distance = 300.0f;
    float azimuth = 0.0f;
    float elevation = 0.4f;
    QVector3D focusPoint{0.0f, 0.0f, 0.0f};

    void loadFromJson(const QJsonObject& obj) {
        distance = static_cast<float>(obj.value("distance").toDouble(distance));
        azimuth = static_cast<float>(obj.value("azimuth").toDouble(azimuth));
        elevation = static_cast<float>(obj.value("elevation").toDouble(elevation));

        if (obj.contains("focusPoint")) {
            QJsonArray arr = obj.value("focusPoint").toArray();
            if (arr.size() >= 3) {
                focusPoint = QVector3D(
                    static_cast<float>(arr[0].toDouble()),
                    static_cast<float>(arr[1].toDouble()),
                    static_cast<float>(arr[2].toDouble())
                );
            }
        }
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["distance"] = static_cast<double>(distance);
        obj["azimuth"] = static_cast<double>(azimuth);
        obj["elevation"] = static_cast<double>(elevation);
        obj["focusPoint"] = QJsonArray{
            static_cast<double>(focusPoint.x()),
            static_cast<double>(focusPoint.y()),
            static_cast<double>(focusPoint.z())
        };
        return obj;
    }
};

} // namespace RSConfig
