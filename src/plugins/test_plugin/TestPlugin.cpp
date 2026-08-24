#include "TestPlugin.hpp"
#include <QDebug>

namespace mnemis::plugins::test {

QString TestPlugin::name() const {
    return "TestPlugin";
}

void TestPlugin::onStartup() {
    qInfo() << "[TestPlugin] Plugin started up!";
}

void TestPlugin::onMediaLoaded(const QString& mediaId) {
    qInfo() << "[TestPlugin] Media loaded with ID:" << mediaId;
}

} // namespace mnemis::plugins::test
