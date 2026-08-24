#pragma once
#include "plugins/api/IPlugin.hpp"
#include <QObject>
#include <QtPlugin>

namespace mnemis::plugins::test {

class TestPlugin : public QObject, public IPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IPlugin_iid)
    Q_INTERFACES(mnemis::plugins::IPlugin)

public:
    QString name() const override;
    void onStartup() override;
    void onMediaLoaded(const QString& mediaId) override;
};

} // namespace mnemis::plugins::test
