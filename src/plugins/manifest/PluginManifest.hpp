#pragma once
#include <QString>
#include <QStringList>
#include <QJsonObject>

namespace mnemis::plugins {

struct PluginManifest {
    QString id;
    QString name;
    QString version;
    QString vendor;
    QString description;
    QString apiVersion;
    QString entryPoint;
    QString platform;
    QString architecture;
    QStringList permissions;
    QStringList capabilities;
    QStringList dependencies;

    bool isValid() const;
    static PluginManifest fromJson(const QJsonObject& json);
};

} // namespace mnemis::plugins
