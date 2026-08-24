#pragma once
#include <QString>
#include <QtPlugin>

namespace mnemis::plugins {

class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual QString name() const = 0;
    virtual void onStartup() = 0;
    virtual void onMediaLoaded(const QString& mediaId) = 0;
};

} // namespace mnemis::plugins

#define IPlugin_iid "com.aleniastudios.Mnemis.IPlugin/1.0"
Q_DECLARE_INTERFACE(mnemis::plugins::IPlugin, IPlugin_iid)
