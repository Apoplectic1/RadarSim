#include "AppSettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDebug>

namespace RSConfig {

AppSettings::AppSettings(QObject* parent)
    : QObject(parent)
{
    // Ensure config directories exist
    QDir().mkpath(configDir());
    QDir().mkpath(profilesDir());
}

QString AppSettings::configDir() const
{
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return appData;
}

QString AppSettings::profilesDir() const
{
    return configDir() + "/profiles";
}

QString AppSettings::profilePath(const QString& name) const
{
    return profilesDir() + "/" + name + ".json";
}

QString AppSettings::lastSessionPath() const
{
    return configDir() + "/last_session.json";
}

bool AppSettings::saveToFile(const QString& path) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "AppSettings: Failed to open file for writing:" << path;
        return false;
    }

    QJsonDocument doc(toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool AppSettings::loadFromFile(const QString& path)
{
    QFile file(path);
    if (!file.exists()) {
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "AppSettings: Failed to open file for reading:" << path;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "AppSettings: JSON parse error:" << parseError.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "AppSettings: Invalid JSON structure";
        return false;
    }

    loadFromJson(doc.object());
    return true;
}

QJsonObject AppSettings::toJson() const
{
    QJsonObject obj;
    obj["version"] = kConfigVersion;
    obj["beam"] = beam.toJson();
    obj["camera"] = camera.toJson();
    obj["target"] = target.toJson();
    obj["scene"] = scene.toJson();
    return obj;
}

void AppSettings::loadFromJson(const QJsonObject& obj)
{
    // Check version for future compatibility
    int version = obj.value("version").toInt(1);
    if (version > kConfigVersion) {
        qWarning() << "AppSettings: Config version" << version
                   << "is newer than supported version" << kConfigVersion;
    }

    if (obj.contains("beam")) {
        beam.loadFromJson(obj.value("beam").toObject());
    }
    if (obj.contains("camera")) {
        camera.loadFromJson(obj.value("camera").toObject());
    }
    if (obj.contains("target")) {
        target.loadFromJson(obj.value("target").toObject());
    }
    if (obj.contains("scene")) {
        scene.loadFromJson(obj.value("scene").toObject());
    }
}

bool AppSettings::saveProfile(const QString& name)
{
    if (name.isEmpty()) {
        qWarning() << "AppSettings: Cannot save profile with empty name";
        return false;
    }

    QString path = profilePath(name);
    if (saveToFile(path)) {
        currentProfile_ = name;
        emit settingsSaved();
        emit profilesChanged();
        return true;
    }
    return false;
}

bool AppSettings::loadProfile(const QString& name)
{
    if (name.isEmpty()) {
        qWarning() << "AppSettings: Cannot load profile with empty name";
        return false;
    }

    QString path = profilePath(name);
    if (loadFromFile(path)) {
        currentProfile_ = name;
        emit settingsLoaded();
        return true;
    }
    return false;
}

bool AppSettings::deleteProfile(const QString& name)
{
    if (name.isEmpty()) {
        return false;
    }

    QString path = profilePath(name);
    QFile file(path);
    if (file.exists() && file.remove()) {
        if (currentProfile_ == name) {
            currentProfile_.clear();
        }
        emit profilesChanged();
        return true;
    }
    return false;
}

bool AppSettings::renameProfile(const QString& oldName, const QString& newName)
{
    if (oldName.isEmpty() || newName.isEmpty()) {
        return false;
    }

    QString oldPath = profilePath(oldName);
    QString newPath = profilePath(newName);

    QFile file(oldPath);
    if (file.exists() && file.rename(newPath)) {
        if (currentProfile_ == oldName) {
            currentProfile_ = newName;
        }
        emit profilesChanged();
        return true;
    }
    return false;
}

QStringList AppSettings::availableProfiles() const
{
    QDir dir(profilesDir());
    QStringList filters;
    filters << "*.json";
    QStringList files = dir.entryList(filters, QDir::Files, QDir::Name);

    // Remove .json extension
    QStringList profiles;
    for (const QString& file : files) {
        profiles << file.left(file.length() - 5);
    }
    return profiles;
}

void AppSettings::saveLastSession()
{
    saveToFile(lastSessionPath());
}

bool AppSettings::restoreLastSession()
{
    if (loadFromFile(lastSessionPath())) {
        currentProfile_.clear();  // Last session is not a named profile
        emit settingsLoaded();
        return true;
    }
    return false;
}

void AppSettings::resetToDefaults()
{
    beam = BeamConfig();
    camera = CameraConfig();
    target = TargetConfig();
    scene = SceneConfig();
    currentProfile_.clear();
    emit settingsLoaded();
}

} // namespace RSConfig
