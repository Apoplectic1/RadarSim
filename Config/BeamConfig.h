#pragma once

#include <QJsonObject>
#include <QJsonArray>
#include <QVector3D>

namespace RSConfig {

struct BeamConfig {
    int beamType = 3;           // 0=Conical, 1=Shaped, 2=Phased, 3=Sinc (default)
    float beamWidth = 15.0f;    // degrees
    float opacity = 0.3f;
    QVector3D color{1.0f, 0.5f, 0.0f};
    bool visible = true;
    int direction = 0;          // 0=ToOrigin, 1=Away, 2=Custom

    void loadFromJson(const QJsonObject& obj) {
        beamType = obj.value("type").toInt(beamType);
        beamWidth = static_cast<float>(obj.value("width").toDouble(beamWidth));
        opacity = static_cast<float>(obj.value("opacity").toDouble(opacity));
        visible = obj.value("visible").toBool(visible);
        direction = obj.value("direction").toInt(direction);

        if (obj.contains("color")) {
            QJsonArray arr = obj.value("color").toArray();
            if (arr.size() >= 3) {
                color = QVector3D(
                    static_cast<float>(arr[0].toDouble()),
                    static_cast<float>(arr[1].toDouble()),
                    static_cast<float>(arr[2].toDouble())
                );
            }
        }
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["type"] = beamType;
        obj["width"] = static_cast<double>(beamWidth);
        obj["opacity"] = static_cast<double>(opacity);
        obj["visible"] = visible;
        obj["direction"] = direction;
        obj["color"] = QJsonArray{
            static_cast<double>(color.x()),
            static_cast<double>(color.y()),
            static_cast<double>(color.z())
        };
        return obj;
    }
};

} // namespace RSConfig
