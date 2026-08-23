#include "SettingsViewModel.hpp"
#include <QUrl>

namespace mnemis::ui::controllers {

SettingsViewModel::SettingsViewModel(core::IConfig* config, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_includeHidden(false)
{
    if (m_config) {
        loadFromConfig();
    }
}

QStringList SettingsViewModel::libraryRoots() const {
    return m_roots;
}

bool SettingsViewModel::includeHidden() const {
    return m_includeHidden;
}

void SettingsViewModel::setIncludeHidden(bool include) {
    if (m_includeHidden != include) {
        m_includeHidden = include;
        saveToConfig();
        emit includeHiddenChanged();
        notifyChanges();
    }
}

void SettingsViewModel::addRoot(const QString& path) {
    QString localPath = QUrl(path).isLocalFile() ? QUrl(path).toLocalFile() : path;
    if (!m_roots.contains(localPath)) {
        m_roots.append(localPath);
        saveToConfig();
        emit libraryRootsChanged();
        notifyChanges();
    }
}

void SettingsViewModel::removeRoot(const QString& path) {
    QString localPath = QUrl(path).isLocalFile() ? QUrl(path).toLocalFile() : path;
    if (m_roots.removeOne(localPath)) {
        saveToConfig();
        emit libraryRootsChanged();
        notifyChanges();
    }
}

void SettingsViewModel::toggleHidden() {
    setIncludeHidden(!m_includeHidden);
}

void SettingsViewModel::loadFromConfig() {
    if (!m_config) return;

    auto configRoots = m_config->getStringList("indexer.libraryRoots");
    m_roots.clear();
    for (const auto& r : configRoots) {
        m_roots.append(QString::fromStdString(r));
    }
    m_includeHidden = m_config->getBool("indexer.includeHidden", false);
}

void SettingsViewModel::saveToConfig() {
    if (!m_config) return;

    std::vector<std::string> configRoots;
    configRoots.reserve(m_roots.size());
    for (const auto& r : m_roots) {
        configRoots.push_back(r.toStdString());
    }
    
    m_config->setStringList("indexer.libraryRoots", configRoots);
    m_config->setBool("indexer.includeHidden", m_includeHidden);
}

void SettingsViewModel::notifyChanges() {
    if (onSettingsChanged) {
        std::vector<std::string> roots;
        roots.reserve(m_roots.size());
        for (const auto& r : m_roots) {
            roots.push_back(r.toStdString());
        }
        onSettingsChanged(roots, m_includeHidden);
    }
}

} // namespace mnemis::ui::controllers
