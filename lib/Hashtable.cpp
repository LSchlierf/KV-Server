#include <string>
#include <vector>
#include <shared_mutex>
#include <mutex>
#include <stdexcept>
#include <iostream>
#include <optional>

template <class K, class V>
class Hashtable
{
private:
    struct list_element
    {
        K key;
        V value;
        struct list_element *next;

        list_element(K key, V value, struct list_element *next) : key(key), value(value), next(next) {}
    };

    std::vector<list_element *> table;
    std::vector<std::shared_mutex *> locks;
    int size;

    size_t hash(K key)
    {
        return std::hash<K>{}(key) % size;
    }

public:
    Hashtable(int size)
    {
        if (size < 1)
        {
            throw std::invalid_argument(" Hashtable size has to be greater than 0");
        }
        this->size = size;
        table.resize(size);
        for(int i = 0; i < size; i++) {
            locks.push_back(new std::shared_mutex());
        }
    }

    bool insert(K key, V value)
    {
        size_t bucket = hash(key);
        std::unique_lock lock(*(locks[bucket]));
        struct list_element *cur = table[bucket];

        while (cur)
        {
            if (cur->key == key)
            {
                return false;
            }
            cur = cur->next;
        }

        struct list_element *new_element = new struct list_element(key, value, table[bucket]);

        table[bucket] = new_element;

        return true;
    }

    std::optional<V> get(K key)
    {
        size_t bucket = hash(key);
        std::shared_lock lock(*(locks[bucket]));
        struct list_element *cur = table[bucket];

        while (cur)
        {
            if (cur->key == key)
            {
                return cur->value;
            }
            cur = cur->next;
        }

        return {};
    }

    bool remove(K key)
    {
        size_t bucket = hash(key);
        std::unique_lock lock(*(locks[bucket]));
        struct list_element *cur = table[bucket];
        struct list_element *pre = nullptr;

        while (cur)
        {
            if (cur->key == key)
            {
                if (cur == table[bucket])
                {
                    table[bucket] = cur->next;
                }
                else
                {
                    pre->next = cur->next;
                }
                delete cur;
                return true;
            }
            pre = cur;
            cur = cur->next;
        }

        return false;
    }

    ~Hashtable()
    {
        for (struct list_element * cur : table)
        {
            while (cur)
            {
                struct list_element *tmp = cur;
                cur = cur->next;
                delete tmp;
            }
        }
    }
};
