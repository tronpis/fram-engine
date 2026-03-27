#include "JobSystem.h"
#include "core/logger/Logger.h"

namespace farm {

JobSystem::JobSystem() = default;

JobSystem::~JobSystem() {
    if (m_running) {
        shutdown();
    }
}

bool JobSystem::init() {
    if (m_running) {
        FARM_LOG_WARN("JobSystem already initialized");
        return true;
    }

    const unsigned int numThreads = std::max(1u, std::thread::hardware_concurrency());
    FARM_LOG_INFO("Initializing JobSystem with {} worker threads", numThreads);

    m_running = true;

    for (unsigned int i = 0; i < numThreads; ++i) {
        m_workers.emplace_back(&JobSystem::workerThread, this);
    }

    return true;
}

void JobSystem::shutdown() {
    if (!m_running) {
        return;
    }

    FARM_LOG_INFO("Shutting down JobSystem...");

    m_running = false;
    m_condition.notify_all();

    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    m_workers.clear();
    FARM_LOG_INFO("JobSystem shutdown complete");
}

void JobSystem::addJob(std::function<void()> job) {
    if (!m_running) {
        FARM_LOG_ERROR("JobSystem not running, cannot add job");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_jobQueue.push(std::move(job));
        m_pendingJobs++;
    }
    m_condition.notify_one();
}

void JobSystem::waitForAllJobs() {
    std::unique_lock<std::mutex> lock(m_completionMutex);
    m_completionCondition.wait(lock, [this] {
        return m_pendingJobs.load() == 0;
    });
}

void JobSystem::workerThread() {
    while (true) {
        std::function<void()> job;

        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_condition.wait(lock, [this] {
                return !m_jobQueue.empty() || !m_running;
            });

            if (!m_running && m_jobQueue.empty()) {
                return;
            }

            if (!m_jobQueue.empty()) {
                job = std::move(m_jobQueue.front());
                m_jobQueue.pop();
            }
        }

        if (job) {
            job();
            int remaining = --m_pendingJobs;
            if (remaining == 0) {
                m_completionCondition.notify_all();
            }
            }
        }
    }
}

} // namespace farm
