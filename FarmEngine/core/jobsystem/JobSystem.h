#pragma once

#include <atomic>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace farm {

/**
 * @brief JobSystem for multithreaded task execution
 */
class JobSystem {
public:
    JobSystem();
    ~JobSystem();

    bool init();
    void shutdown();

    void addJob(std::function<void()> job);
    void waitForAllJobs();

private:
    void workerThread();

    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_jobQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_running{false};
    std::atomic<int> m_pendingJobs{0};
    std::mutex m_completionMutex;
    std::condition_variable m_completionCondition;
};

} // namespace farm
