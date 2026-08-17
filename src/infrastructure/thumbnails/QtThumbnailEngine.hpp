#pragma once

#include "core/thumbnails/IThumbnailEngine.hpp"
#include "core/thumbnails/IThumbnailCache.hpp"
#include "core/thumbnails/IThumbnailDecoder.hpp"
#include <QThreadPool>
#include <QObject>
#include <memory>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>

namespace mnemis::infrastructure::thumbnails {

class QtThumbnailEngine : public core::thumbnails::IThumbnailEngine {
public:
    QtThumbnailEngine(std::shared_ptr<core::thumbnails::IThumbnailCache> cache);
    ~QtThumbnailEngine() override;

    void addDecoder(std::shared_ptr<core::thumbnails::IThumbnailDecoder> decoder);

    core::thumbnails::ThumbnailTaskId requestThumbnail(
        const core::thumbnails::ThumbnailSpec& spec,
        core::thumbnails::ThumbnailPriority priority,
        core::thumbnails::ThumbnailCallback callback) override;

    void cancelRequest(core::thumbnails::ThumbnailTaskId taskId) override;
    void clearPendingRequests() override;

private:
    struct TaskContext {
        std::atomic<bool> cancelled{false};
        core::thumbnails::ThumbnailSpec spec;
        core::thumbnails::ThumbnailCallback callback;
    };

    class ThumbnailRunnable;

    std::shared_ptr<core::thumbnails::IThumbnailCache> m_cache;
    std::vector<std::shared_ptr<core::thumbnails::IThumbnailDecoder>> m_decoders;

    QThreadPool m_threadPool;
    
    std::mutex m_tasksMutex;
    core::thumbnails::ThumbnailTaskId m_nextTaskId = 1;
    // Map of currently active or queued tasks
    std::unordered_map<core::thumbnails::ThumbnailTaskId, std::shared_ptr<TaskContext>> m_activeTasks;
    
    // We track total tasks to apply backpressure
    std::atomic<int> m_taskCount{0};
    const int MAX_QUEUE_SIZE = 1000;
    
    void onTaskFinished(core::thumbnails::ThumbnailTaskId taskId);
};

} // namespace mnemis::infrastructure::thumbnails
