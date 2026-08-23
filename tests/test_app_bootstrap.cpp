#include <gtest/gtest.h>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QGuiApplication>
#include <QTemporaryDir>

#include "ui/components/MpvVideoItem.hpp"
#include "infrastructure/bootstrap/DefaultLibraryRoots.hpp"

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

TEST_F(AppBootstrapTest, DefaultLibraryRoots) {
    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());
    const QString existing = temporary.path() + "/Pictures";
    ASSERT_TRUE(QDir().mkpath(existing));
    const QString missing = temporary.path() + "/Missing";

    const QStringList roots = infrastructure::bootstrap::existingDefaultLibraryRoots(
        {existing, missing, existing});
    EXPECT_EQ(roots, QStringList({existing}));
}
