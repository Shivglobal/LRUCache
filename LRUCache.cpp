#include <iostream>
#include <list>
#include <variant>
#include<unordered_map>

using namespace std;
class LRUCache
{
    private:
        using Node = std::pair<int,int>;
        int cap_;
        std::list<Node> cache_;
        std::unordered_map<int,std::list<Node>::iterator> map_;
        void moveToFront(std::list<Node>::iterator it)
        {
            cache_.splice(cache_.begin(), cache_ , it);
        }
    public:
        LRUCache( int cap): cap_(cap){}
        int get (int key)
        {
            auto it = map_.find(key);
            
            if(it == map_.end())
                return -1;
            moveToFront(it->second);
            return it->second->second;
        }
        void put(int key, int val)
        {
            auto it = map_.find(key);
            if(it != map_.end())
            {
                it->second->second = val;
                moveToFront(it->second);
                return;
            }
            if(cache_.size() == cap_)
            {
                auto last = cache_.back();
                map_.erase(last.first);
                cache_.pop_back();
            }
            cache_.emplace_front(key,val);
            map_[key] = cache_.begin();
        }
        void display()
        {
            std::cout << "Cache state ( MRU --- > LRU )";
            for ( auto &p : cache_)
            {
                std::cout << "[ " << p.first << ", " << p.second << "]";
            }
            std::cout << std::endl;
        }
};

int main()
{
    LRUCache cache(3);
    cache.put(1,10);
    cache.put(2,20);
    cache.put(3,30);
    cache.display();
    std::cout << "cache.get (2) : " << cache.get(2) <<std::endl;
    cache.display();
    cache.put(4,40);
    cache.display();
    std::cout << " cache.get(1) : " << cache.get(1) << std::endl;
    cache.display();
    std::cout<<" cache.get (3) : " << cache.get(3) << std::endl;
    cache.display();
    std::cout<< std::endl;
    
    
    
    std::variant<int, std::string> v = 42;
    std::visit([](auto&&val){
        std::cout<<" value of the variant store : " << val <<std::endl;
    },v);
    std::visit([](auto&& arg) {
        std::cout << arg;
    }, v);
}
