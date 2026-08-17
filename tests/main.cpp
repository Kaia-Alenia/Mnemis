#include <gtest/gtest.h>
#include <QDir>
#include <QCoreApplication>

inline void initTestResources() {
    Q_INIT_RESOURCE(database);
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    initTestResources();
    return RUN_ALL_TESTS();
}

