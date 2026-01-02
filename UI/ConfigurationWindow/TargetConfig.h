#pragma once

#include <QJsonObject>
#include <QJsonArray>
#include <QVector3D>

namespace RSConfig {

struct TargetConfig {
    int targetType = 0;         // 0=Cube, 1=Cylinder, 2=Aircraft, 3=Sphere
    QVector3D position{0.0f, 0.0f, 0.0f};
    QVector3D rotation{0.0f, 0.0f, 0.0f};  // Euler angles in degrees
    float scale = 20.0f;
    QVector3D color{0.0f, 1.0f, 0.0f};
    bool visible = true;

    void loadFromJson(const QJsonObject& obj) {
        targetType = obj.value("type").toInt(targetType);
        scale = static_cast<float>(obj.value("scale").toDouble(scale));
        visible = obj.value("visible").toBool(visible);

        if (obj.contains("position")) {
            QJsonArray arr = obj.value("position").toArray();
            if (arr.size() >= 3) {
                position = QVector3D(
                    static_cast<float>(arr[0].toDouble()),
                    static_cast<float>(arr[1].toDouble()),
                    static_cast<float>(arr[2].toDouble())
                );
            }
        }

        if (obj.contains("rotation")) {
            QJsonArray arr = obj.value("rotation").toArray();
            if (arr.size() >= 3) {
                rotation = QVector3D(
                    static_cast<float>(arr[0].toDouble()),
                    static_cast<float>(arr[1].toDouble()),
                    static_cast<float>(arr[2].toDouble())
                );
            }
        }

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
        obj["type"] = targetType;
        obj["scale"] = static_cast<double>(scale);
        obj["visible"] = visible;
        obj["position"] = QJsonArray{
            static_cast<double>(position.x()),
            static_cast<double>(position.y()),
            static_cast<double>(position.z())
        };
        obj["rotation"] = QJsonArray{
            static_cast<double>(rotation.x()),
            static_cast<double>(rotation.y()),
            static_cast<double>(rotation.z())
        };
        obj["color"] = QJsonArray{
            static_cast<double>(color.x()),
            static_cast<double>(color.y()),
            static_cast<double>(color.z())
        };
        return obj;
    }
};

} // namespace RSConfig
