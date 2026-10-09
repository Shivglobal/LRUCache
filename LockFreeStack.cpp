#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

template <typename T>
class LockFreeStack
{
private:
    struct Node
    {
        T data;
        Node* next;

        Node(const T& value)
            : data(value), next(nullptr)
        {
        }
    };

    std::atomic<Node*> top{nullptr};

public:

    // Push an element
    void push(const T& value)
    {
        Node* newNode = new Node(value);

        // Get current top
        newNode->next = top.load(std::memory_order_relaxed);

        // Try to make newNode the new top
        while (!top.compare_exchange_weak(
            newNode->next,              // expected
            newNode,                    // desired
            std::memory_order_release,
            std::memory_order_relaxed))
        {
            // CAS failed.
            // newNode->next has been updated
            // with the current top.
        }
    }

    // Pop an element
    bool pop(T& result)
    {
        Node* oldTop = top.load(std::memory_order_acquire);

        while (oldTop != nullptr)
        {
            Node* next = oldTop->next;

            // Try to change top from oldTop to next
            if (top.compare_exchange_weak(
                    oldTop,             // expected
                    next,               // desired
                    std::memory_order_acquire,
                    std::memory_order_relaxed))
            {
                result = oldTop->data;

               // delete oldTop;

                return true;
            }

            // CAS failed.
            // oldTop is automatically updated with
            // the current value of top.
        }

        return false; // Stack is empty
    }

    ~LockFreeStack()
    {
        Node* node = top.load();

        while (node != nullptr)
        {
            Node* next = node->next;
            delete node;
            node = next;
        }
    }
};


int main()
{
    LockFreeStack<int> stack;

    // -------------------------
    // Single-threaded test
    // -------------------------

    std::cout << "Single-threaded test\n";

    stack.push(10);
    stack.push(20);
    stack.push(30);

    int value;

    while (stack.pop(value))
    {
        std::cout << "Popped: " << value << '\n';
    }


    // -------------------------
    // Multi-threaded test
    // -------------------------

    std::cout << "\nMulti-threaded test\n";

    LockFreeStack<int> multiStack;

    constexpr int NUM_THREADS = 4;
    constexpr int VALUES_PER_THREAD = 1000;

    std::vector<std::thread> producers;

    // Multiple producers
    for (int t = 0; t < NUM_THREADS; ++t)
    {
        producers.emplace_back(
            [&multiStack, t]()
            {
                for (int i = 0; i < VALUES_PER_THREAD; ++i)
                {
                    multiStack.push(
                        t * VALUES_PER_THREAD + i
                    );
                }
            });
    }

    // Wait for producers
    for (auto& thread : producers)
    {
        thread.join();
    }

    std::cout << "All producers finished\n";

    // -------------------------
    // Multiple consumers
    // -------------------------

    std::atomic<int> popCount{0};

    std::vector<std::thread> consumers;

    for (int t = 0; t < NUM_THREADS; ++t)
    {
        consumers.emplace_back(
            [&multiStack, &popCount]()
            {
                int value;

                while (multiStack.pop(value))
                {
                    ++popCount;
                }
            });
    }

    // Wait for consumers
    for (auto& thread : consumers)
    {
        thread.join();
    }

    std::cout << "Total popped: "
              << popCount.load()
              << '\n';

    std::cout << "Expected: "
              << NUM_THREADS * VALUES_PER_THREAD
              << '\n';

    return 0;
}
