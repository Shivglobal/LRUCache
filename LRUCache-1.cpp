#include <iostream>
#include <list>
#include <unordered_map>

class LRUCache
{
private:
    using Node = std::pair<int, int>;
    int cap_;
    std::list<Node> cache_;
    std::unordered_map<int, std::list<Node>::iterator> map_;

    void moveToFront(std::list<Node>::iterator it)
    {
        cache_.splice(cache_.begin(), cache_, it);
    }

public:
    explicit LRUCache(int cap) : cap_(cap) {}

    int get(int key)
    {
        auto it = map_.find(key);
        if (it == map_.end())
            return -1; // Cache miss
            
        moveToFront(it->second);
        return it->second->second; // Return value
    }

    void put(int key, int val)
    {
        auto it = map_.find(key);
        if (it != map_.end())
        {
            // Key already exists: update value and move to front (MRU)
            it->second->second = val;
            moveToFront(it->second);
            return;
        }

        // If cache is at capacity, evict the least recently used (back) element
        if (cache_.size() == static_cast<size_t>(cap_))
        {
            auto last = cache_.back();
            map_.erase(last.first);
            cache_.pop_back();
        }

        // Insert new element at the front (MRU)
        cache_.emplace_front(key, val);
        map_[key] = cache_.begin();
    }

    void display() const
    {
        std::cout << "Cache state (MRU ---> LRU): ";
        for (const auto &p : cache_)
        {
            std::cout << "[" << p.first << ": " << p.second << "] ";
        }
        std::cout << "\n";
    }
};

int main()
{
    // Create an LRU cache with a small capacity of 3
    LRUCache cache(3);

    std::cout << "=== 1. Inserting 3 items (Capacity: 3) ===\n";
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    cache.display(); // Expected: [3: 30] [2: 20] [1: 10]

    std::cout << "\n=== 2. Accessing key 2 (makes it MRU) ===\n";
    std::cout << "cache.get(2) = " << cache.get(2) << "\n";
    cache.display(); // Expected: [2: 20] [3: 30] [1: 10]

    std::cout << "\n=== 3. Inserting a 4th item (should evict LRU item: key 1) ===\n";
    cache.put(4, 40);
    cache.display(); // Expected: [4: 40] [2: 20] [3: 30]

    std::cout << "\n=== 4. Testing cache miss for evicted item (key 1) ===\n";
    std::cout << "cache.get(1) = " << cache.get(1) << " (Expected: -1)\n";

    std::cout << "\n=== 5. Accessing key 3 and updating it ===\n";
    std::cout << "cache.get(3) = " << cache.get(3) << "\n";
    cache.put(3, 33); // Update value
    cache.display(); // Expected: [3: 33] [4: 40] [2: 20]

    return 0;
}
