#include "orion/ThreadPool.h"
#include <juce_core/juce_core.h>

namespace Orion {

    ThreadPool& ThreadPool::global() {
        static ThreadPool pool;
        return pool;
    }

    ThreadPool::ThreadPool(size_t threads)
        : stop(false)
    {

        if (threads < 2) threads = 2;

        for(size_t i = 0; i<threads; ++i)
            workers.emplace_back(
                [this, i] {
                    juce::Thread::setCurrentThreadName("OrionWorker-" + std::to_string(i));
                    // juce::Thread::setCurrentThreadPriority(10); // High priority (Removed in JUCE 8)

                    for(;;) {
                        std::function<void()> task;

                        {
                            std::unique_lock<std::mutex> lock(this->queue_mutex);
                            this->condition.wait(lock,
                                [this]{ return this->stop || !this->tasks.empty(); });
                            if(this->stop && this->tasks.empty())
                                return;
                            task = std::move(this->tasks.front());
                            this->tasks.pop();
                        }

                        task();
                    }
                }
            );
    }

    ThreadPool::~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for(std::thread &worker: workers) {
            if (worker.joinable())
                worker.join();
        }
    }

}
