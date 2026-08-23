#pragma once

#include <QObject>
#include <QStringList>
#include <vector>
#include <string>
#include <functional>
#include "core/IConfig.hpp"

namespace mnemis::ui::controllers {

class SettingsViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList libraryRoots READ libraryRoots NOTIFY libraryRootsChanged)
    Q_PROPERTY(bool includeHidden READ includeHidden WRITE setIncludeHidden NOTIFY includeHiddenChanged)

public:
    explicit SettingsViewModel(core::IConfig* config, QObject* parent = nullptr);

    QStringList libraryRoots() const;
    bool includeHidden() const;
    Q_INVOKABLE void setIncludeHidden(bool include);

    Q_INVOKABLE void addRoot(const QString& path);
    Q_INVOKABLE void removeRoot(const QString& path);
    Q_INVOKABLE void toggleHidden();

    // Callback to notify the application layer that re-indexing is requested
    std::function<void(const std::vector<std::string>& roots, bool includeHidden)> onSettingsChanged;

signals:
    void libraryRootsChanged();
    void includeHiddenChanged();

private:
    core::IConfig* m_config;
    QStringList m_roots;
    bool m_includeHidden;

    void loadFromConfig();
    void saveToConfig();
    void notifyChanges();
};

} // namespace mnemis::ui::controllers
