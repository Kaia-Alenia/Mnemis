#include "PluginDiscovery.hpp"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

namespace mnemis::plugins {

PluginDiscovery::PluginDiscovery(const QString& pluginsDir)
    : m_pluginsDir(pluginsDir) {}

QVector<PluginManifest> PluginDiscovery::discover() {
    QVector<PluginManifest> manifests;
    QDir dir(m_pluginsDir);

    if (!dir.exists()) {
        qWarning() << "Plugin directory does not exist:" << m_pluginsDir;
        return manifests;
    }

    // Iterate through all subdirectories (each plugin is assumed to be in its own folder)
    for (const QString& subDirName : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QDir pluginDir(dir.absoluteFilePath(subDirName));
        QString manifestPath = pluginDir.absoluteFilePath("manifest.json");

        if (QFile::exists(manifestPath)) {
            QFile file(manifestPath);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QByteArray data = file.readAll();
                file.close();

                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

                if (parseError.error != QJsonParseError::NoError) {
                    qWarning() << "Failed to parse manifest.json in" << subDirName << ":" << parseError.errorString();
                    continue;
                }

                if (doc.isObject()) {
                    PluginManifest manifest = PluginManifest::fromJson(doc.object());
                    if (manifest.isValid()) {
                        manifests.append(manifest);
                    } else {
                        qWarning() << "Invalid manifest in" << subDirName;
                    }
                }
            } else {
                qWarning() << "Failed to open manifest.json in" << subDirName;
            }
        }
    }

    return manifests;
}

} // namespace mnemis::plugins
