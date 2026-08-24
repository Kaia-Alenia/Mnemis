#include "LibraryController.hpp"

#include <QUrl>

namespace mnemis::ui {

namespace {

QString uiMediaTypeName(
    media::MediaType type
)
{
    switch (type) {
        case media::MediaType::Image:
            return QStringLiteral("image");

        case media::MediaType::Video:
            return QStringLiteral("video");

        case media::MediaType::Audio:
            return QStringLiteral("audio");

        case media::MediaType::Unknown:
        default:
            return QStringLiteral("unknown");
    }
}

}

LibraryController::LibraryController(
    library::LibraryService& service,
    media::ThumbnailService& thumbnails,
    QObject* parent
)
    : QObject(parent)
    , m_service(service)
    , m_thumbnails(thumbnails)
{
}

QVariantList LibraryController::roots() const
{
    return m_roots;
}

QVariantList LibraryController::folders() const
{
    return m_folders;
}

QVariantList LibraryController::media() const
{
    return m_media;
}

qulonglong LibraryController::selectedRootId() const
{
    return m_selectedRootId;
}

qulonglong LibraryController::currentFolderId() const
{
    return m_currentFolderId;
}

QString LibraryController::currentFolderPath() const
{
    return m_currentFolderPath;
}

QString LibraryController::currentFolderName() const
{
    return m_currentFolderName;
}

bool LibraryController::atRoot() const
{
    return m_atRoot;
}

QString LibraryController::errorMessage() const
{
    return m_errorMessage;
}

void LibraryController::setError(
    const QString& message
)
{
    if (m_errorMessage == message) {
        return;
    }

    m_errorMessage = message;

    emit errorMessageChanged();
}

void LibraryController::clearError()
{
    if (m_errorMessage.isEmpty()) {
        return;
    }

    m_errorMessage.clear();

    emit errorMessageChanged();
}

void LibraryController::clearBrowser()
{
    m_currentFolderId = 0;
    m_currentFolderPath.clear();
    m_currentFolderName.clear();
    m_atRoot = true;

    m_folders.clear();
    m_media.clear();

    emit currentFolderChanged();
    emit foldersChanged();
    emit mediaChanged();
}

QVariantMap LibraryController::rootToMap(
    const library::LibraryRoot& root
) const
{
    QVariantMap map;

    map.insert(
        QStringLiteral("id"),
        QVariant::fromValue<qulonglong>(
            root.id
        )
    );

    map.insert(
        QStringLiteral("path"),
        QString::fromStdString(
            root.path.string()
        )
    );

    map.insert(
        QStringLiteral("name"),
        QString::fromStdString(
            root.name
        )
    );

    map.insert(
        QStringLiteral("enabled"),
        root.enabled
    );

    return map;
}

QVariantMap LibraryController::folderToMap(
    const library::Folder& folder
) const
{
    QVariantMap map;

    map.insert(
        QStringLiteral("id"),
        QVariant::fromValue<qulonglong>(
            folder.id
        )
    );

    map.insert(
        QStringLiteral("rootId"),
        QVariant::fromValue<qulonglong>(
            folder.rootId
        )
    );

    map.insert(
        QStringLiteral("parentId"),
        QVariant::fromValue<qulonglong>(
            folder.parentId
        )
    );

    map.insert(
        QStringLiteral("path"),
        QString::fromStdString(
            folder.path.string()
        )
    );

    map.insert(
        QStringLiteral("name"),
        QString::fromStdString(
            folder.name
        )
    );

    map.insert(
        QStringLiteral("isRoot"),
        folder.isRootFolder
    );

    return map;
}

QVariantMap LibraryController::mediaToMap(
    const media::Media& media
) const
{
    QVariantMap map;

    map.insert(
        QStringLiteral("id"),
        QVariant::fromValue<qulonglong>(
            media.id
        )
    );

    map.insert(
        QStringLiteral("path"),
        QString::fromStdString(
            media.canonicalPath
        )
    );

    map.insert(
        QStringLiteral("name"),
        QString::fromStdString(
            media.fileName
        )
    );

    map.insert(
        QStringLiteral("extension"),
        QString::fromStdString(
            media.extension
        )
    );

    map.insert(
        QStringLiteral("type"),
        uiMediaTypeName(
            media.type
        )
    );

    map.insert(
        QStringLiteral("size"),
        QVariant::fromValue<qlonglong>(
            media.fileSize
        )
    );

    map.insert(
        QStringLiteral("modifiedTime"),
        QVariant::fromValue<qlonglong>(
            media.modifiedTime
        )
    );

    map.insert(
        QStringLiteral("favorite"),
        media.favorite
    );

    QString thumbnailUrl;

    if (media.type == media::MediaType::Image) {
        auto thumbnailResult =
            m_thumbnails.thumbnailFor(
                media,
                256,
                256
            );

        if (thumbnailResult.isSuccess()) {
            thumbnailUrl =
                QUrl::fromLocalFile(
                    QString::fromStdString(
                        thumbnailResult.value().string()
                    )
                ).toString();
        }
    }

    map.insert(
        QStringLiteral("thumbnailUrl"),
        thumbnailUrl
    );

    return map;
}

void LibraryController::refreshRoots()
{
    clearError();

    auto result =
        m_service.getRoots(true);

    if (result.isError()) {
        setError(
            QString::fromStdString(
                result.error().message()
            )
        );

        return;
    }

    m_roots.clear();

    bool selectedStillExists = false;

    for (const auto& root :
         result.value()) {

        m_roots.append(
            rootToMap(root)
        );

        if (root.id ==
            static_cast<library::LibraryRootId>(
                m_selectedRootId
            )) {
            selectedStillExists = true;
        }
    }

    emit rootsChanged();

    if (!selectedStillExists) {
        if (m_roots.isEmpty()) {
            m_selectedRootId = 0;
        } else {
            const QVariantMap first =
                m_roots.first().toMap();

            m_selectedRootId =
                first.value(
                    QStringLiteral("id")
                ).toULongLong();
        }

        emit selectedRootChanged();
    }

    refreshBrowser();
}

void LibraryController::selectRoot(
    qulonglong rootId
)
{
    clearError();

    if (rootId == 0) {
        setError(
            QStringLiteral(
                "Invalid library root."
            )
        );

        return;
    }

    auto result =
        m_service.getRoot(
            static_cast<library::LibraryRootId>(
                rootId
            )
        );

    if (result.isError()) {
        setError(
            QString::fromStdString(
                result.error().message()
            )
        );

        return;
    }

    if (m_selectedRootId != rootId) {
        m_selectedRootId = rootId;

        emit selectedRootChanged();
    }

    goToRoot();
}

bool LibraryController::addRoot(
    const QString& path,
    const QString& name
)
{
    clearError();

    const QString trimmedPath =
        path.trimmed();

    const QString trimmedName =
        name.trimmed();

    if (trimmedPath.isEmpty()) {
        setError(
            QStringLiteral(
                "Library path cannot be empty."
            )
        );

        return false;
    }

    if (trimmedName.isEmpty()) {
        setError(
            QStringLiteral(
                "Library name cannot be empty."
            )
        );

        return false;
    }

    auto result =
        m_service.addRoot(
            filesystem::Path(
                trimmedPath.toStdString()
            ),
            trimmedName.toStdString()
        );

    if (result.isError()) {
        setError(
            QString::fromStdString(
                result.error().message()
            )
        );

        return false;
    }

    const qulonglong newRootId =
        static_cast<qulonglong>(
            result.value()
        );

    refreshRoots();

    if (m_selectedRootId != newRootId) {
        m_selectedRootId = newRootId;

        emit selectedRootChanged();

        goToRoot();
    }

    return true;
}

bool LibraryController::removeSelectedRoot()
{
    clearError();

    if (m_selectedRootId == 0) {
        setError(
            QStringLiteral(
                "No library root is selected."
            )
        );

        return false;
    }

    auto result =
        m_service.removeRoot(
            static_cast<library::LibraryRootId>(
                m_selectedRootId
            )
        );

    if (result.isError()) {
        setError(
            QString::fromStdString(
                result.error().message()
            )
        );

        return false;
    }

    m_selectedRootId = 0;

    emit selectedRootChanged();

    refreshRoots();

    return true;
}

bool LibraryController::setSelectedRootEnabled(
    bool enabled
)
{
    clearError();

    if (m_selectedRootId == 0) {
        setError(
            QStringLiteral(
                "No library root is selected."
            )
        );

        return false;
    }

    auto result =
        m_service.setRootEnabled(
            static_cast<library::LibraryRootId>(
                m_selectedRootId
            ),
            enabled
        );

    if (result.isError()) {
        setError(
            QString::fromStdString(
                result.error().message()
            )
        );

        return false;
    }

    refreshRoots();

    return true;
}

bool LibraryController::indexSelectedRoot()
{
    clearError();

    if (m_selectedRootId == 0) {
        setError(
            QStringLiteral(
                "No library root is selected."
            )
        );

        return false;
    }

    auto result =
        m_service.indexRoot(
            static_cast<library::LibraryRootId>(
                m_selectedRootId
            )
        );

    if (result.isError()) {
        setError(
            QString::fromStdString(
                result.error().message()
            )
        );

        return false;
    }

    refreshBrowser();

    return true;
}

void LibraryController::refreshBrowser()
{
    clearError();

    if (m_selectedRootId == 0) {
        clearBrowser();
        return;
    }

    if (!loadRootFolder()) {
        return;
    }

    if (m_currentFolderId == 0) {
        return;
    }

    loadChildren();
    refreshMedia();
}

bool LibraryController::loadRootFolder()
{
    auto result =
        m_service.foldersInRoot(
            static_cast<library::LibraryRootId>(
                m_selectedRootId
            )
        );

    if (result.isError()) {
        setError(
            QString::fromStdString(
                result.error().message()
            )
        );

        return false;
    }

    library::Folder rootFolder;

    bool found = false;

    for (const auto& folder :
         result.value()) {

        if (folder.isRootFolder) {
            rootFolder = folder;
            found = true;
            break;
        }
    }

    if (!found) {
        auto rootResult =
            m_service.getRoot(
                static_cast<library::LibraryRootId>(
                    m_selectedRootId
                )
            );

        if (rootResult.isError()) {
            setError(
                QString::fromStdString(
                    rootResult.error().message()
                )
            );

            return false;
        }

        m_currentFolderId = 0;

        m_currentFolderPath =
            QString::fromStdString(
                rootResult.value().path.string()
            );

        m_currentFolderName =
            QString::fromStdString(
                rootResult.value().name
            );

        m_atRoot = true;

        m_folders.clear();

        emit currentFolderChanged();
        emit foldersChanged();

        return true;
    }

    m_currentFolderId =
        static_cast<qulonglong>(
            rootFolder.id
        );

    m_currentFolderPath =
        QString::fromStdString(
            rootFolder.path.string()
        );

    m_currentFolderName =
        QString::fromStdString(
            rootFolder.name
        );

    m_atRoot = true;

    emit currentFolderChanged();

    return true;
}

bool LibraryController::loadChildren()
{
    m_folders.clear();

    if (m_selectedRootId == 0) {
        emit foldersChanged();
        return false;
    }

    if (m_currentFolderId == 0) {
        emit foldersChanged();
        return true;
    }

    auto result =
        m_service.childFolders(
            static_cast<library::LibraryRootId>(
                m_selectedRootId
            ),
            static_cast<library::FolderId>(
                m_currentFolderId
            )
        );

    if (result.isError()) {
        setError(
            QString::fromStdString(
                result.error().message()
            )
        );

        emit foldersChanged();

        return false;
    }

    for (const auto& folder :
         result.value()) {

        m_folders.append(
            folderToMap(folder)
        );
    }

    emit foldersChanged();

    return true;
}

void LibraryController::goToRoot()
{
    clearError();

    if (m_selectedRootId == 0) {
        clearBrowser();
        return;
    }

    if (!loadRootFolder()) {
        return;
    }

    loadChildren();
    refreshMedia();
}

void LibraryController::openFolder(
    qulonglong folderId
)
{
    clearError();

    if (m_selectedRootId == 0 ||
        folderId == 0) {

        setError(
            QStringLiteral(
                "Invalid folder."
            )
        );

        return;
    }

    auto result =
        m_service.foldersInRoot(
            static_cast<library::LibraryRootId>(
                m_selectedRootId
            )
        );

    if (result.isError()) {
        setError(
            QString::fromStdString(
                result.error().message()
            )
        );

        return;
    }

    bool found = false;

    for (const auto& folder :
         result.value()) {

        if (folder.id ==
            static_cast<library::FolderId>(
                folderId
            )) {

            m_currentFolderId =
                folderId;

            m_currentFolderPath =
                QString::fromStdString(
                    folder.path.string()
                );

            m_currentFolderName =
                QString::fromStdString(
                    folder.name
                );

            m_atRoot =
                folder.isRootFolder;

            found = true;

            break;
        }
    }

    if (!found) {
        setError(
            QStringLiteral(
                "Folder not found."
            )
        );

        return;
    }

    emit currentFolderChanged();

    loadChildren();
    refreshMedia();
}

void LibraryController::goUp()
{
    clearError();

    if (m_selectedRootId == 0) {
        return;
    }

    if (m_atRoot ||
        m_currentFolderId == 0) {

        return;
    }

    auto result =
        m_service.foldersInRoot(
            static_cast<library::LibraryRootId>(
                m_selectedRootId
            )
        );

    if (result.isError()) {
        setError(
            QString::fromStdString(
                result.error().message()
            )
        );

        return;
    }

    library::Folder currentFolder;

    bool foundCurrent = false;

    for (const auto& folder :
         result.value()) {

        if (folder.id ==
            static_cast<library::FolderId>(
                m_currentFolderId
            )) {

            currentFolder =
                folder;

            foundCurrent = true;
            break;
        }
    }

    if (!foundCurrent) {
        goToRoot();
        return;
    }

    if (currentFolder.parentId == 0) {
        goToRoot();
        return;
    }

    openFolder(
        static_cast<qulonglong>(
            currentFolder.parentId
        )
    );
}

void LibraryController::refreshMedia()
{
    clearError();

    m_media.clear();

    if (m_selectedRootId == 0) {
        emit mediaChanged();
        return;
    }

    core::Result<std::vector<media::Media>> result =
        m_atRoot
            ? m_service.mediaInRoot(
                  static_cast<library::LibraryRootId>(
                      m_selectedRootId
                  ),
                  false
              )
            : m_service.mediaInFolder(
                  static_cast<library::LibraryRootId>(
                      m_selectedRootId
                  ),
                  filesystem::Path(
                      m_currentFolderPath.toStdString()
                  ),
                  false
              );

    if (result.isError()) {
        setError(
            QString::fromStdString(
                result.error().message()
            )
        );

        emit mediaChanged();

        return;
    }

    for (const auto& item :
         result.value()) {

        m_media.append(
            mediaToMap(item)
        );
    }

    emit mediaChanged();
}

} // namespace mnemis::ui
