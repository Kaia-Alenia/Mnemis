#include "QtThumbnailEngine.hpp"
#include <QRunnable>
#include <QFileInfo>
#include <algorithm>
#include <iostream>

namespace mnemis::infrastructure::thumbnails {

class QtThumbnailEngine::ThumbnailRunnable : public QRunnable {
public:
    ThumbnailRunnable(
        QtThumbnailEngine* engine,
        core::thumbnails::ThumbnailTaskId taskId,
        std::shared_ptr<TaskContext> context
    )
        : m_engine(engine), m_taskId(taskId), m_context(context) {
        setAutoDelete(true);
    }

    void run() override {
        // RAII to ensure cleanup
        struct Cleanup {
            QtThumbnailEngine* eng;
            core::thumbnails::ThumbnailTaskId tid;
            ~Cleanup() { eng->onTaskFinished(tid); }
        } cleanup{m_engine, m_taskId};

        if (m_context->cancelled) {
            notify(core::thumbnails::ThumbnailStatus::Cancelled, std::nullopt);
            return;
        }

        std::string key = m_engine->m_cache->generateKey(m_context->spec);
        
        // 1. Try cache
        auto cached = m_engine->m_cache->load(key);
        if (cached) {
            if (m_context->cancelled) {
                notify(core::thumbnails::ThumbnailStatus::Cancelled, std::nullopt);
                return;
            }
            notify(core::thumbnails::ThumbnailStatus::Ready, std::move(cached));
            return;
        }

        // 2. Decode
        std::shared_ptr<core::thumbnails::IThumbnailDecoder> chosenDecoder;
        QFileInfo fi(QString::fromStdString(m_context->spec.canonicalPath));
        std::string ext = "." + fi.suffix().toStdString();
        
        for (auto& dec : m_engine->m_decoders) {
            if (dec->canHandle(ext)) {
                chosenDecoder = dec;
                break;
            }
        }

        if (!chosenDecoder) {
            notify(core::thumbnails::ThumbnailStatus::Unavailable, std::nullopt);
            return;
        }

        if (m_context->cancelled) {
            notify(core::thumbnails::ThumbnailStatus::Cancelled, std::nullopt);
            return;
        }

        auto decoded = chosenDecoder->decode(m_context->spec.canonicalPath, m_context->spec);
        if (!decoded) {
            notify(core::thumbnails::ThumbnailStatus::Error, std::nullopt);
            return;
        }

        // 3. Save to cache
        m_engine->m_cache->save(key, *decoded);

        if (m_context->cancelled) {
            notify(core::thumbnails::ThumbnailStatus::Cancelled, std::nullopt);
            return;
        }

        notify(core::thumbnails::ThumbnailStatus::Ready, std::move(decoded));
    }

private:
    void notify(core::thumbnails::ThumbnailStatus status, std::optional<core::thumbnails::ImageBuffer> img) {
        if (!m_context->callback) return;
        
        core::thumbnails::ThumbnailResult res;
        res.mediaId = m_context->spec.mediaId;
        res.status = status;
        if (img) {
            res.image = std::move(img);
            res.width = res.image->width;
            res.height = res.image->height;
        }
        
        m_context->callback(res);
    }

    QtThumbnailEngine* m_engine;
    core::thumbnails::ThumbnailTaskId m_taskId;
    std::shared_ptr<TaskContext> m_context;
};

QtThumbnailEngine::QtThumbnailEngine(std::shared_ptr<core::thumbnails::IThumbnailCache> cache)
    : m_cache(std::move(cache)) {
    // Determine a reasonable number of threads based on CPU, but reserve some for UI/DB
    int idealThreads = QThread::idealThreadCount();
    int threadCount = std::max(2, idealThreads - 2); 
    m_threadPool.setMaxThreadCount(threadCount);
}

QtThumbnailEngine::~QtThumbnailEngine() {
    clearPendingRequests();
    // Block until all running threads see the cancelled flag and exit
    m_threadPool.waitForDone();
}

void QtThumbnailEngine::addDecoder(std::shared_ptr<core::thumbnails::IThumbnailDecoder> decoder) {
    m_decoders.push_back(std::move(decoder));
}

core::thumbnails::ThumbnailTaskId QtThumbnailEngine::requestThumbnail(
    const core::thumbnails::ThumbnailSpec& spec,
    core::thumbnails::ThumbnailPriority priority,
    core::thumbnails::ThumbnailCallback callback) {
    
    // Backpressure check
    if (m_taskCount.load() >= MAX_QUEUE_SIZE) {
        if (priority == core::thumbnails::ThumbnailPriority::Background ||
            priority == core::thumbnails::ThumbnailPriority::Prefetch) {
            
            // Reject silently by invoking cancelled or unavailable immediately
            if (callback) {
                core::thumbnails::ThumbnailResult res;
                res.mediaId = spec.mediaId;
                res.status = core::thumbnails::ThumbnailStatus::Cancelled;
                callback(res);
            }
            return 0; // Return invalid ID
        }
    }

    auto ctx = std::make_shared<TaskContext>();
    ctx->spec = spec;
    ctx->callback = callback;

    core::thumbnails::ThumbnailTaskId taskId = 0;
    {
        std::lock_guard<std::mutex> lock(m_tasksMutex);
        taskId = m_nextTaskId++;
        m_activeTasks[taskId] = ctx;
        m_taskCount++;
    }

    // Map priority enum to QThreadPool priority (higher number = higher priority)
    int qtPriority = 0;
    switch (priority) {
        case core::thumbnails::ThumbnailPriority::Visible: qtPriority = 100; break;
        case core::thumbnails::ThumbnailPriority::NearVisible: qtPriority = 50; break;
        case core::thumbnails::ThumbnailPriority::Prefetch: qtPriority = 25; break;
        case core::thumbnails::ThumbnailPriority::Background: qtPriority = 0; break;
    }

    auto* runnable = new ThumbnailRunnable(this, taskId, ctx);
    m_threadPool.start(runnable, qtPriority);

    return taskId;
}

void QtThumbnailEngine::cancelRequest(core::thumbnails::ThumbnailTaskId taskId) {
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    auto it = m_activeTasks.find(taskId);
    if (it != m_activeTasks.end()) {
        it->second->cancelled = true;
    }
}

void QtThumbnailEngine::clearPendingRequests() {
    // 1. Mark all active contexts as cancelled so running threads stop early
    {
        std::lock_guard<std::mutex> lock(m_tasksMutex);
        for (auto& pair : m_activeTasks) {
            pair.second->cancelled = true;
        }
    }
    
    // 2. Remove items from the queue that haven't started executing yet.
    // QThreadPool::clear() removes tasks that are not running yet.
    m_threadPool.clear();
}

// Internal method called by Runnable's RAII
void QtThumbnailEngine::onTaskFinished(core::thumbnails::ThumbnailTaskId taskId) {
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    m_activeTasks.erase(taskId);
    m_taskCount--;
}

} // namespace mnemis::infrastructure::thumbnails
