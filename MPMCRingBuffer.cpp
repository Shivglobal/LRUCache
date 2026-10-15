#include <atomic>
#include <vector>
#include <cassert>

template<typename T>
class MPMCRingBuffer
{
private:

    struct Cell
    {
        std::atomic<size_t> sequence;
        T data;
    };

    std::vector<Cell> buffer;

    const size_t capacity;
    const size_t mask;

    // Separate cache lines to avoid false sharing
    alignas(64) std::atomic<size_t> enqueuePos{0};

    alignas(64) std::atomic<size_t> dequeuePos{0};

public:

    explicit MPMCRingBuffer(size_t size)
        : buffer(size),
          capacity(size),
          mask(size - 1)
    {
        assert((size >= 2) && ((size & (size - 1)) == 0));

        for (size_t i = 0; i < capacity; i++)
            buffer[i].sequence.store(i, std::memory_order_relaxed);
    }

    //---------------------------------------------------------
    // Push
    //---------------------------------------------------------

    bool push(const T& item)
    {
        Cell* cell;
        size_t pos = enqueuePos.load(std::memory_order_relaxed);

        while (true)
        {
            cell = &buffer[pos & mask];

            size_t seq =
                cell->sequence.load(std::memory_order_acquire);

            intptr_t diff =
                (intptr_t)seq - (intptr_t)pos;

            //-------------------------------------------------
            // Slot available
            //-------------------------------------------------

            if (diff == 0)
            {
                if (enqueuePos.compare_exchange_weak(
                        pos,
                        pos + 1,
                        std::memory_order_relaxed))
                    break;
            }

            //-------------------------------------------------
            // Queue Full
            //-------------------------------------------------

            else if (diff < 0)
            {
                return false;
            }

            //-------------------------------------------------
            // Another producer won
            //-------------------------------------------------

            else
            {
                pos =
                    enqueuePos.load(std::memory_order_relaxed);
            }
        }

        cell->data = item;

        cell->sequence.store(
            pos + 1,
            std::memory_order_release);

        return true;
    }

    //---------------------------------------------------------
    // Pop
    //---------------------------------------------------------

    bool pop(T& item)
    {
        Cell* cell;

        size_t pos =
            dequeuePos.load(std::memory_order_relaxed);

        while (true)
        {
            cell = &buffer[pos & mask];

            size_t seq =
                cell->sequence.load(std::memory_order_acquire);

            intptr_t diff =
                (intptr_t)seq - (intptr_t)(pos + 1);

            //-------------------------------------------------
            // Item available
            //-------------------------------------------------

            if (diff == 0)
            {
                if (dequeuePos.compare_exchange_weak(
                        pos,
                        pos + 1,
                        std::memory_order_relaxed))
                    break;
            }

            //-------------------------------------------------
            // Queue Empty
            //-------------------------------------------------

            else if (diff < 0)
            {
                return false;
            }

            //-------------------------------------------------
            // Another consumer won
            //-------------------------------------------------

            else
            {
                pos =
                    dequeuePos.load(std::memory_order_relaxed);
            }
        }

        item = cell->data;

        cell->sequence.store(
            pos + capacity,
            std::memory_order_release);

        return true;
    }
};
