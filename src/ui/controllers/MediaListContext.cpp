#include "MediaListContext.hpp"
#include <QThreadPool>
#include <QMetaObject>

namespace mnemis::ui::controllers {

MediaListContext::MediaListContext(core::repositories::IMediaRepository* repository, QObject* parent)
    : QObject(parent)
    , m_repository(repository)
    , m_isAlive(std::make_shared<bool>(true))
{
    m_loadTimer.setSingleShot(true);
    connect(&m_loadTimer, &QTimer::timeout, this, &MediaListContext::doLoad);
    refreshTotalCount();
}

MediaListContext::~MediaListContext() {
    *m_isAlive = false;
    if (m_eventBus && m_eventSubToken) {
        m_eventBus->unsubscribe(m_eventSubToken);
    }
}

void MediaListContext::setEventBus(std::shared_ptr<core::events::ILibraryEventBus> eventBus) {
    if (m_eventBus && m_eventSubToken) {
        m_eventBus->unsubscribe(m_eventSubToken);
    }
    m_eventBus = std::move(eventBus);
    if (m_eventBus) {
        auto alive = m_isAlive;
        m_eventSubToken = m_eventBus->subscribe([this, alive](const core::events::LibraryEvent& event) {
            if (!*alive) return;
            auto self = const_cast<MediaListContext*>(this);
            QMetaObject::invokeMethod(self, [self, event]() {
                self->onLibraryEvent(event);
            }, Qt::QueuedConnection);
        });
    }
}

int MediaListContext::totalCount() const { return m_totalCount; }
int MediaListContext::currentIndex() const { return m_currentIndex; }
QString MediaListContext::currentMediaId() const { return m_currentMediaId; }
bool MediaListContext::hasNext() const { return m_currentIndex >= 0 && m_currentIndex < m_totalCount - 1; }
bool MediaListContext::hasPrevious() const { return m_currentIndex > 0; }

core::repositories::QueryOptions MediaListContext::currentQuery() const { return m_query; }

void MediaListContext::setQueryOptions(const core::repositories::QueryOptions& query) {
    m_query = query;
    m_currentIndex = -1;
    m_currentMediaId.clear();
    emit queryChanged();
    emit currentIndexChanged();
    emit currentMediaIdChanged();
    emit hasNextChanged();
    emit hasPreviousChanged();
    refreshTotalCount();
}

void MediaListContext::refreshTotalCount() {
    int currentGen = ++m_navGeneration;
    auto alive = m_isAlive;
    auto repo = m_repository;
    auto query = m_query;
    
    QThreadPool::globalInstance()->start([this, alive, repo, query, currentGen]() {
        auto result = repo->count(query);
        if (!*alive) return;
        
        int count = result.isSuccess() ? result.value() : 0;
        
        auto self = const_cast<MediaListContext*>(this);
        QMetaObject::invokeMethod(self, [self, currentGen, count]() {
            if (currentGen != self->m_navGeneration) return;
            self->m_totalCount = count;
            emit self->totalCountChanged();
            emit self->hasNextChanged();
            emit self->hasPreviousChanged();
        }, Qt::QueuedConnection);
    });
}

void MediaListContext::open(const QString& mediaId, int indexHint) {
    m_currentMediaId = mediaId;
    m_currentIndex = indexHint;
    emit currentMediaIdChanged();
    emit currentIndexChanged();
    emit hasNextChanged();
    emit hasPreviousChanged();
}

void MediaListContext::next() {
    if (!hasNext()) return;
    
    m_currentIndex++; // Optimistic update
    emit currentIndexChanged();
    emit hasNextChanged();
    emit hasPreviousChanged();

    m_loadTimer.start(50);
}

void MediaListContext::previous() {
    if (!hasPrevious()) return;

    m_currentIndex--; // Optimistic update
    emit currentIndexChanged();
    emit hasNextChanged();
    emit hasPreviousChanged();

    m_loadTimer.start(50);
}

void MediaListContext::doLoad() {
    int targetIndex = m_currentIndex;
    int gen = ++m_navGeneration;
    auto alive = m_isAlive;
    auto repo = m_repository;
    auto query = m_query;
    
    QThreadPool::globalInstance()->start([this, alive, repo, query, targetIndex, gen]() {
        auto res = repo->list(targetIndex + 1, 1, query); // SQLite uses 1-based page
        if (!*alive) return;
        
        if (res.isSuccess() && !res.value().empty()) {
            auto item = res.value().front();
            auto self = const_cast<MediaListContext*>(this);
            QMetaObject::invokeMethod(self, [self, gen, item]() {
                if (gen != self->m_navGeneration) return;
                self->m_currentMediaId = QString::fromStdString(item.mediaId);
                emit self->currentMediaIdChanged();
            }, Qt::QueuedConnection);
        }
    });
}

void MediaListContext::onLibraryEvent(const core::events::LibraryEvent& event) {
    using namespace core::events;
    if (event.type == LibraryEvent::Type::Added || 
        event.type == LibraryEvent::Type::Removed || 
        event.type == LibraryEvent::Type::BatchAdded || 
        event.type == LibraryEvent::Type::BatchRemoved) {
        refreshTotalCount();
    }
}

} // namespace mnemis::ui::controllers
