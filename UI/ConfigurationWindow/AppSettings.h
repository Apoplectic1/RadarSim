#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

#include "BeamConfig.h"
#include "CameraConfig.h"
#include "TargetConfig.h"
#include "SceneConfig.h"

namespace RSConfig {

class AppSettings : public QObject {
    Q_OBJECT

public:
    explicit AppSettings(QObject* parent = nullptr);
    ~AppSettings() override = default;

    // Configuration data
    BeamConfig beam;
    CameraConfig camera;
    TargetConfig target;
    SceneConfig scene;

    // Profile management
    bool saveProfile(const QString& name);
    bool loadProfile(const QString& name);
    bool deleteProfile(const QString& name);
    QStringList availableProfiles() const;
    QString currentProfileName() const { return currentProfile_; }

    // Session persistence (auto-save/restore)
    void saveLastSession();
    bool restoreLastSession();

    // Reset to defaults
    void resetToDefaults();

    // JSON serialization
    QJsonObject toJson() const;
    void loadFromJson(const QJsonObject& obj);

signals:
    void profilesChanged();

private:
    QString configDir() const;
    QString profilesDir() const;
    QString profilePath(const QString& name) const;
    QString lastSessionPath() const;

    bool saveToFile(const QString& path) const;
    bool loadFromFile(const QString& path);

    QString currentProfile_;
    static constexpr int kConfigVersion = 1;
};

} // namespace RSConfig
