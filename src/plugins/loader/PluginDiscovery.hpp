#pragma once
#include <QString>
#include <QVector>
#include "plugins/manifest/PluginManifest.hpp"

namespace mnemis::plugins {

class PluginDiscovery {
public:
    PluginDiscovery(const QString& pluginsDir);
    
    // Discovers plugins in the directory and parses their manifests
    QVector<PluginManifest> discover();

private:
    QString m_pluginsDir;
};

} // namespace mnemis::plugins
