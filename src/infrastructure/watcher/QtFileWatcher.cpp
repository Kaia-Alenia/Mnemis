#include "QtFileWatcher.hpp"
#include <QDirIterator>
#include <QFileInfo>
#include <QDebug>
#include <vector>

namespace mnemis::infrastructure::watcher {

QtFileWatcher::QtFileWatcher(int debounceIntervalMs, QObject* parent)
    : QObject(parent)
    , m_debounceIntervalMs(debounceIntervalMs)
{
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &QtFileWatcher::onDirectoryChanged);
}

QtFileWatcher::~QtFileWatcher() {
    // Child objects like QTimer are automatically deleted.
}

void QtFileWatcher::watchDirectory(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    addRecursively(QString::fromStdString(path));
}

void QtFileWatcher::unwatchDirectory(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    removeRecursively(QString::fromStdString(path));
}

void QtFileWatcher::setCallback(core::watcher::FileWatcherCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callback = std::move(callback);
}

void QtFileWatcher::addRecursively(const QString& path) {
    QFileInfo info(path);
    if (!info.exists() || !info.isDir()) {
        return;
    }

    if (m_watchedDirectories.find(path) == m_watchedDirectories.end()) {
        if (!m_watcher.addPath(path)) {
            qWarning() << "Failed to add watch for directory (limit reached or permission denied):" << path;
            return;
        }
        m_watchedDirectories.insert(path);
    }

    QDirIterator it(path, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString subDir = it.next();
        if (m_watchedDirectories.find(subDir) == m_watchedDirectories.end()) {
            if (!m_watcher.addPath(subDir)) {
                qWarning() << "Failed to add watch for subdirectory (limit reached or permission denied):" << subDir;
            } else {
                m_watchedDirectories.insert(subDir);
            }
        }
    }
}

void QtFileWatcher::removeRecursively(const QString& path) {
    std::vector<QString> toRemove;
    for (const auto& watched : m_watchedDirectories) {
        if (watched == path || watched.startsWith(path + "/")) {
            toRemove.push_back(watched);
        }
    }

    for (const auto& removePath : toRemove) {
        m_watcher.removePath(removePath);
        m_watchedDirectories.erase(removePath);
        
        auto it = m_debouncers.find(removePath);
        if (it != m_debouncers.end()) {
            it->second->stop();
            it->second->deleteLater();
            m_debouncers.erase(it);
        }
    }
}

void QtFileWatcher::onDirectoryChanged(const QString& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    QFileInfo info(path);
    if (!info.exists()) {
        // Directory was removed
        removeRecursively(path);
        triggerDebounce(path);
        return;
    }

    // Check for new subdirectories created inside this directory
    QDirIterator it(path, QDir::Dirs | QDir::NoDotAndDotDot);
    while (it.hasNext()) {
        QString subDir = it.next();
        if (m_watchedDirectories.find(subDir) == m_watchedDirectories.end()) {
            addRecursively(subDir);
        }
    }

    triggerDebounce(path);
}

void QtFileWatcher::triggerDebounce(const QString& path) {
    auto it = m_debouncers.find(path);
    if (it == m_debouncers.end()) {
        QTimer* timer = new QTimer(this);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, this, [this, path]() {
            std::string stdPath = path.toStdString();
            core::watcher::FileWatcherCallback cb;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                cb = m_callback;
            }
            if (cb) {
                cb(stdPath);
            }
        });
        m_debouncers[path] = timer;
        timer->start(m_debounceIntervalMs);
    } else {
        it->second->start(m_debounceIntervalMs);
    }
}

} // namespace mnemis::infrastructure::watcher
