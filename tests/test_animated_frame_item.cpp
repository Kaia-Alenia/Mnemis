#include <gtest/gtest.h>
#include <QSignalSpy>

#include "ui/components/AnimatedFrameItem.hpp"
#include "ui/controllers/AnimatedMediaController.hpp"

using namespace mnemis::ui;

TEST(AnimatedFrameItemTest, StartsWithoutController) {
    components::AnimatedFrameItem item;
    EXPECT_EQ(item.controller(), nullptr);
}

TEST(AnimatedFrameItemTest, EmitsControllerChangedWhenControllerChanges) {
    components::AnimatedFrameItem item;
    controllers::AnimatedMediaController controller;

    QSignalSpy controllerChangedSpy(&item, &components::AnimatedFrameItem::controllerChanged);
    item.setController(&controller);

    EXPECT_EQ(item.controller(), &controller);
    EXPECT_EQ(controllerChangedSpy.count(), 1);
}

TEST(AnimatedFrameItemTest, SettingSameControllerDoesNotEmitSignalAgain) {
    components::AnimatedFrameItem item;
    controllers::AnimatedMediaController controller;

    QSignalSpy controllerChangedSpy(&item, &components::AnimatedFrameItem::controllerChanged);
    item.setController(&controller);
    item.setController(&controller);

    EXPECT_EQ(controllerChangedSpy.count(), 1);
}

TEST(AnimatedFrameItemTest, ClearingControllerEmitsSignalAndResetsPointer) {
    components::AnimatedFrameItem item;
    controllers::AnimatedMediaController controller;

    QSignalSpy controllerChangedSpy(&item, &components::AnimatedFrameItem::controllerChanged);
    item.setController(&controller);
    item.setController(nullptr);

    EXPECT_EQ(item.controller(), nullptr);
    EXPECT_EQ(controllerChangedSpy.count(), 2);
}