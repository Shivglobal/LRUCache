#include <iostream>
#include <array>
#include <atomic>

template<typename T, size_t N>
class RingBuffer {
private:
    std::array<T, N> buffer;
    std::atomic<size_t> head{0};
    std::atomic<size_t> tail{0};

public:
    bool push(const T& item) {
        size_t currentTail = tail.load(std::memory_order_relaxed);
        size_t nextTail = (currentTail + 1) % N;

        if (nextTail == head.load(std::memory_order_acquire))
            return false; // Buffer full

        buffer[currentTail] = item;

        tail.store(nextTail, std::memory_order_release);

        return true;
    }

    bool pop(T& item) {
        size_t currentHead = head.load(std::memory_order_relaxed);

        if (currentHead == tail.load(std::memory_order_acquire))
            return false; // Buffer empty

        item = buffer[currentHead];

        head.store((currentHead + 1) % N,
                   std::memory_order_release);

        return true;
    }
};

int main() {
    RingBuffer<int, 5> rb;

    // Push elements
    for (int i = 1; i <= 4; ++i) {
        if (rb.push(i * 10))
            std::cout << "Pushed: " << i * 10 << '\n';
        else
            std::cout << "Buffer Full! Could not push " << i * 10 << '\n';
    }

    // Try pushing one more element
    if (!rb.push(50))
        std::cout << "Buffer Full! Could not push 50\n";

    std::cout << "\nPopping elements:\n";

    int value;
    while (rb.pop(value)) {
        std::cout << "Popped: " << value << '\n';
    }

    if (!rb.pop(value))
        std::cout << "Buffer Empty!\n";

    return 0;
}
