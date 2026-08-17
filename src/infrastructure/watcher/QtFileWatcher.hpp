#pragma once

#include "core/watcher/IFileWatcher.hpp"
#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>
#include <QString>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <mutex>

namespace mnemis::infrastructure::watcher {

/**
 * @brief Qt-based implementation of IFileWatcher.
 * 
 * Uses QFileSystemWatcher to monitor directories. Since QFileSystemWatcher
 * is not recursive by default, this class explicitly discovers and watches
 * subdirectories. It also coalesces (debounces) bursts of events using QTimer.
 */
class QtFileWatcher : public QObject, public core::watcher::IFileWatcher {
    Q_OBJECT
public:
    explicit QtFileWatcher(int debounceIntervalMs = 1000, QObject* parent = nullptr);
    ~QtFileWatcher() override;

    void watchDirectory(const std::string& path) override;
    void unwatchDirectory(const std::string& path) override;
    void setCallback(core::watcher::FileWatcherCallback callback) override;

private slots:
    void onDirectoryChanged(const QString& path);

private:
    void addRecursively(const QString& path);
    void removeRecursively(const QString& path);
    void triggerDebounce(const QString& path);

    QFileSystemWatcher m_watcher;
    core::watcher::FileWatcherCallback m_callback;
    int m_debounceIntervalMs;
    
    // Timers for debouncing per path
    std::unordered_map<QString, QTimer*> m_debouncers;
    // Set of directories we are explicitly tracking (to handle deletions gracefully)
    std::unordered_set<QString> m_watchedDirectories;
    
    std::mutex m_mutex;
};

} // namespace mnemis::infrastructure::watcher
