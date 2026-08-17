#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include <atomic>
#include <optional>
#include "core/repositories/IMediaRepository.hpp"
#include "core/events/ILibraryEventBus.hpp"

namespace mnemis::ui::controllers {

class MediaListContext : public QObject {
    Q_OBJECT
    Q_PROPERTY(int totalCount READ totalCount NOTIFY totalCountChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QString currentMediaId READ currentMediaId NOTIFY currentMediaIdChanged)
    Q_PROPERTY(bool hasNext READ hasNext NOTIFY hasNextChanged)
    Q_PROPERTY(bool hasPrevious READ hasPrevious NOTIFY hasPreviousChanged)

public:
    explicit MediaListContext(core::repositories::IMediaRepository* repository, QObject* parent = nullptr);
    ~MediaListContext() override;

    void setEventBus(std::shared_ptr<core::events::ILibraryEventBus> eventBus);

    int totalCount() const;
    int currentIndex() const;
    QString currentMediaId() const;
    bool hasNext() const;
    bool hasPrevious() const;

    core::repositories::QueryOptions currentQuery() const;
    void setQueryOptions(const core::repositories::QueryOptions& query);

public slots:
    void open(const QString& mediaId, int indexHint = -1);
    void next();
    void previous();
    void refreshTotalCount();

signals:
    void totalCountChanged();
    void currentIndexChanged();
    void currentMediaIdChanged();
    void hasNextChanged();
    void hasPreviousChanged();
    void queryChanged();

private slots:
    void onLibraryEvent(const core::events::LibraryEvent& event);

private:
    core::repositories::IMediaRepository* m_repository;
    core::repositories::QueryOptions m_query;
    int m_totalCount = 0;
    int m_currentIndex = -1;
    QString m_currentMediaId;
    std::shared_ptr<bool> m_isAlive;
    std::atomic<int> m_navGeneration{0};
    
    std::shared_ptr<core::events::ILibraryEventBus> m_eventBus;
    core::events::ILibraryEventBus::SubscriptionToken m_eventSubToken = 0;
};

} // namespace mnemis::ui::controllers
