#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ui/controllers/SettingsViewModel.hpp"
#include "fakes/FakeConfig.hpp"
#include <vector>
#include <string>

using namespace mnemis::ui::controllers;
using namespace mnemis::tests;

class SettingsViewModelTest : public ::testing::Test {
protected:
    FakeConfig config;
    SettingsViewModel* vm;

    void SetUp() override {
        vm = new SettingsViewModel(&config);
    }

    void TearDown() override {
        delete vm;
    }
};

TEST_F(SettingsViewModelTest, LoadsInitialValuesFromConfig) {
    FakeConfig preConfig;
    preConfig.setStringList("indexer.libraryRoots", {"/path/a", "/path/b"});
    preConfig.setBool("indexer.includeHidden", true);
    
    SettingsViewModel viewModel(&preConfig);
    
    EXPECT_EQ(viewModel.libraryRoots().size(), 2);
    EXPECT_TRUE(viewModel.libraryRoots().contains("/path/a"));
    EXPECT_TRUE(viewModel.libraryRoots().contains("/path/b"));
    EXPECT_TRUE(viewModel.includeHidden());
}

TEST_F(SettingsViewModelTest, AddRootUpdatesConfigAndRoots) {
    bool signalEmitted = false;
    QObject::connect(vm, &SettingsViewModel::libraryRootsChanged, [&]() {
        signalEmitted = true;
    });

    vm->addRoot("file:///new/path");

    EXPECT_TRUE(signalEmitted);
    EXPECT_EQ(vm->libraryRoots().size(), 1);
    EXPECT_EQ(vm->libraryRoots().first(), "/new/path");

    auto configRoots = config.getStringList("indexer.libraryRoots");
    EXPECT_EQ(configRoots.size(), 1);
    EXPECT_EQ(configRoots[0], "/new/path");
}

TEST_F(SettingsViewModelTest, RemoveRootUpdatesConfigAndRoots) {
    vm->addRoot("/path/to/remove");
    
    bool signalEmitted = false;
    QObject::connect(vm, &SettingsViewModel::libraryRootsChanged, [&]() {
        signalEmitted = true;
    });

    vm->removeRoot("file:///path/to/remove");

    EXPECT_TRUE(signalEmitted);
    EXPECT_EQ(vm->libraryRoots().size(), 0);

    auto configRoots = config.getStringList("indexer.libraryRoots");
    EXPECT_EQ(configRoots.size(), 0);
}

TEST_F(SettingsViewModelTest, ToggleHiddenUpdatesConfig) {
    bool signalEmitted = false;
    QObject::connect(vm, &SettingsViewModel::includeHiddenChanged, [&]() {
        signalEmitted = true;
    });

    EXPECT_FALSE(vm->includeHidden());
    vm->toggleHidden();

    EXPECT_TRUE(signalEmitted);
    EXPECT_TRUE(vm->includeHidden());
    EXPECT_TRUE(config.getBool("indexer.includeHidden"));
}

TEST_F(SettingsViewModelTest, OnSettingsChangedCallbackIsInvoked) {
    bool callbackInvoked = false;
    vm->onSettingsChanged = [&](const std::vector<std::string>& roots, bool hidden) {
        callbackInvoked = true;
        EXPECT_EQ(roots.size(), 1);
        EXPECT_EQ(roots[0], "/some/path");
        EXPECT_FALSE(hidden);
    };

    vm->addRoot("/some/path");
    EXPECT_TRUE(callbackInvoked);
}
