#include "PluginManifest.hpp"
#include <QJsonArray>
#include <QJsonValue>

namespace mnemis::plugins {

bool PluginManifest::isValid() const {
    return !id.isEmpty() && !name.isEmpty() && !version.isEmpty() &&
           !vendor.isEmpty() && !description.isEmpty() && !apiVersion.isEmpty() &&
           !entryPoint.isEmpty() && !platform.isEmpty() && !architecture.isEmpty();
}

PluginManifest PluginManifest::fromJson(const QJsonObject& json) {
    PluginManifest manifest;
    manifest.id = json["id"].toString();
    manifest.name = json["name"].toString();
    manifest.version = json["version"].toString();
    manifest.vendor = json["vendor"].toString();
    manifest.description = json["description"].toString();
    manifest.apiVersion = json["apiVersion"].toString();
    manifest.entryPoint = json["entryPoint"].toString();
    manifest.platform = json["platform"].toString();
    manifest.architecture = json["architecture"].toString();
    
    if (json.contains("permissions") && json["permissions"].isArray()) {
        QJsonArray arr = json["permissions"].toArray();
        for (const QJsonValue& val : arr) {
            manifest.permissions.append(val.toString());
        }
    }
    
    if (json.contains("capabilities") && json["capabilities"].isArray()) {
        QJsonArray arr = json["capabilities"].toArray();
        for (const QJsonValue& val : arr) {
            manifest.capabilities.append(val.toString());
        }
    }
    
    if (json.contains("dependencies") && json["dependencies"].isArray()) {
        QJsonArray arr = json["dependencies"].toArray();
        for (const QJsonValue& val : arr) {
            manifest.dependencies.append(val.toString());
        }
    }
    
    return manifest;
}

} // namespace mnemis::plugins
