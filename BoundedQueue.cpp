include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

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
            return false;          // queue shutdown

        value = std::move(buffer.front());
        buffer.pop();

        not_full.notify_one();
        return true;
    }

    void shutdown()
    {
        std::lock_guard<std::mutex> lock(mtx);
        shutdownFlag = true;

        not_full.notify_all();
        not_empty.notify_all();
    }
};

int main()
{
    BoundedQueue<int> q(3);

    std::thread producer([&]()
    {
        for(int i = 1; i <= 10; i++)
        {
            if(!q.push(i))
                break;

            std::cout << "Produced : " << i << '\n';
        }

        q.shutdown();
    });

    std::thread consumer([&]()
    {
        int x;

        while(q.pop(x))
        {
            std::cout << "Consumed : " << x << '\n';
        }

        std::cout << "Consumer exiting\n";
    });

    producer.join();
    consumer.join();
}
