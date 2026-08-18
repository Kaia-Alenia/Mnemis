#include <gtest/gtest.h>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QGuiApplication>

#include "ui/components/MpvVideoItem.hpp"

using namespace mnemis;

class AppBootstrapTest : public ::testing::Test {
protected:
    void SetUp() override {
        // qmlRegisterType must be called before the engine is created
        qmlRegisterType<ui::components::MpvVideoItem>("mnemis.ui", 1, 0, "MpvVideoItem");
    }
};

TEST_F(AppBootstrapTest, RegistersMpvVideoItem) {
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(
        "import QtQuick\n"
        "import mnemis.ui 1.0\n"
        "MpvVideoItem {}\n",
        QUrl()
    );

    QObject *object = component.create();
    ASSERT_TRUE(object != nullptr) << "Failed to create MpvVideoItem from QML: " << component.errorString().toStdString();
    
    auto *item = qobject_cast<ui::components::MpvVideoItem*>(object);
    ASSERT_TRUE(item != nullptr) << "Created object is not of type MpvVideoItem";
    
    delete object;
}
