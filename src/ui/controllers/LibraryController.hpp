#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "library/models/Folder.hpp"
#include "library/models/LibraryRoot.hpp"
#include "library/service/LibraryService.hpp"
#include "media/thumbnails/ThumbnailService.hpp"

namespace mnemis::ui {

class LibraryController final : public QObject {
    Q_OBJECT

    Q_PROPERTY(
        QVariantList roots
        READ roots
        NOTIFY rootsChanged
    )

    Q_PROPERTY(
        QVariantList folders
        READ folders
        NOTIFY foldersChanged
    )

    Q_PROPERTY(
        QVariantList media
        READ media
        NOTIFY mediaChanged
    )

    Q_PROPERTY(
        qulonglong selectedRootId
        READ selectedRootId
        NOTIFY selectedRootChanged
    )

    Q_PROPERTY(
        qulonglong currentFolderId
        READ currentFolderId
        NOTIFY currentFolderChanged
    )

    Q_PROPERTY(
        QString currentFolderPath
        READ currentFolderPath
        NOTIFY currentFolderChanged
    )

    Q_PROPERTY(
        QString currentFolderName
        READ currentFolderName
        NOTIFY currentFolderChanged
    )

    Q_PROPERTY(
        bool atRoot
        READ atRoot
        NOTIFY currentFolderChanged
    )

    Q_PROPERTY(
        QString errorMessage
        READ errorMessage
        NOTIFY errorMessageChanged
    )

public:
    explicit LibraryController(
        library::LibraryService& service,
        media::ThumbnailService& thumbnails,
        QObject* parent = nullptr
    );

    // --------------------------------------------------------
    // Properties
    // --------------------------------------------------------

    QVariantList roots() const;
    QVariantList folders() const;
    QVariantList media() const;

    qulonglong selectedRootId() const;

    qulonglong currentFolderId() const;

    QString currentFolderPath() const;

    QString currentFolderName() const;

    bool atRoot() const;

    QString errorMessage() const;

    // --------------------------------------------------------
    // Root operations
    // --------------------------------------------------------

    Q_INVOKABLE void refreshRoots();

    Q_INVOKABLE void selectRoot(
        qulonglong rootId
    );

    Q_INVOKABLE QString chooseFolder();

    Q_INVOKABLE bool addRoot(
        const QString& path,
        const QString& name
    );

    Q_INVOKABLE bool removeSelectedRoot();

    Q_INVOKABLE bool setSelectedRootEnabled(
        bool enabled
    );

    Q_INVOKABLE bool indexSelectedRoot();

    // --------------------------------------------------------
    // Browser operations
    // --------------------------------------------------------

    Q_INVOKABLE void refreshBrowser();

    Q_INVOKABLE void goToRoot();

    Q_INVOKABLE void openFolder(
        qulonglong folderId
    );

    Q_INVOKABLE void goUp();

    // --------------------------------------------------------
    // Media
    // --------------------------------------------------------

    Q_INVOKABLE void refreshMedia();

signals:
    void rootsChanged();
    void foldersChanged();
    void mediaChanged();

    void selectedRootChanged();
    void currentFolderChanged();

    void errorMessageChanged();

private:
    library::LibraryService& m_service;
    media::ThumbnailService& m_thumbnails;

    QVariantList m_roots;
    QVariantList m_folders;
    QVariantList m_media;

    qulonglong m_selectedRootId = 0;

    qulonglong m_currentFolderId = 0;

    QString m_currentFolderPath;
    QString m_currentFolderName;

    bool m_atRoot = true;

    QString m_errorMessage;

    bool loadRootFolder();

    bool loadChildren();

    void clearBrowser();

    void clearError();

    void setError(
        const QString& message
    );

    QVariantMap rootToMap(
        const library::LibraryRoot& root
    ) const;

    QVariantMap folderToMap(
        const library::Folder& folder
    ) const;

    QVariantMap mediaToMap(
        const media::Media& media
    ) const;
};

} // namespace mnemis::ui
