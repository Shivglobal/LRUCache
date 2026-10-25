#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <string>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <chrono>

template<typename T>
class BoundedQueue
{
private:
    std::queue<T> buffer;
    mutable std::mutex mtx;
    std::condition_variable not_full;
    std::condition_variable not_empty;

    size_t capacity;
    bool shutdownFlag;

public:
    explicit BoundedQueue(size_t cap)
        : capacity(cap), shutdownFlag(false)
    {
    }

    // Producer
    bool push(const T& value)
    {
        std::unique_lock<std::mutex> lock(mtx);

        not_full.wait(lock, [this] {
            return buffer.size() < capacity || shutdownFlag;
        });

        if (shutdownFlag)
            return false;

        buffer.push(value);

        not_empty.notify_one();
        return true;
    }

    // Consumer
    bool pop(T& value)
    {
        std::unique_lock<std::mutex> lock(mtx);

        not_empty.wait(lock, [this] {
            return !buffer.empty() || shutdownFlag;
        });

        if (buffer.empty())
            return false;          // queue shutdown & fully drained

        value = std::move(buffer.front());
        buffer.pop();

        not_full.notify_one();
        return true;
    }

    void shutdown()
    {
        {
            std::lock_guard<std::mutex> lock(mtx);
            shutdownFlag = true;
        } // Mutex is released here, preventing lock contention upon wakeup

        not_full.notify_all();
        not_empty.notify_all();
    }
};

int main()
{
    // Create a bounded queue with a tight capacity of 3 to force blocking behavior
    BoundedQueue<int> q(3);

    const int num_producers = 3;
    const int num_consumers = 2;
    const int items_per_producer = 5;

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    // Launch multiple producers
    for (int p = 0; p < num_producers; ++p)
    {
        producers.emplace_back([&q, p, items_per_producer]() {
            for (int i = 1; i <= items_per_producer; ++i)
            {
                int val = p * 100 + i;
                if (!q.push(val))
                    break;

                std::cout << "[Producer " << p << "] Pushed: " << val << '\n';
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        });
    }

    // Launch multiple consumers
    for (int c = 0; c < num_consumers; ++c)
    {
        consumers.emplace_back([&q, c]() {
            int x;
            while (q.pop(x))
            {
                std::cout << "  [Consumer " << c << "] Popped: " << x << '\n';
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // slower consumer
            }
            std::cout << "  [Consumer " << c << "] Exiting (queue empty & shutdown).\n";
        });
    }

    // Wait for all producers to finish pushing their items
    for (auto& t : producers)
    {
        t.join();
    }

    std::cout << "\n=== All producers finished. Shutting down queue... ===\n\n";
    q.shutdown();

    // Wait for all consumers to drain the remaining items and exit
    for (auto& t : consumers)
    {
        t.join();
    }

    std::cout << "\nAll threads joined successfully. Program completed.\n";
    return 0;
}
