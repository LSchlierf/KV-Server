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
    // linked list bucket element
    struct list_element
    {
        K key;
        V value;
        struct list_element *next;

        list_element(K key, V value, struct list_element *next) : key(key), value(value), next(next) {}
    };

    // linked list buckets
    std::vector<list_element *> table;
    // read-write locks
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
        // prepare buckets and same number of read-write locks
        table.resize(size);
        for (int i = 0; i < size; i++)
        {
            locks.push_back(new std::shared_mutex());
        }
    }

    // inserts a new key value pair, iff key is not already present.
    // returns true iff pair was inserted
    bool insert(K key, V value)
    {
        size_t bucket = hash(key);
        // obtain write lock
        std::unique_lock lock(*(locks[bucket]));

        // verify that key is not present already
        struct list_element *cur = table[bucket];
        while (cur)
        {
            if (cur->key == key)
            {
                return false;
            }
            cur = cur->next;
        }

        // insert key value pair at start of bucket linked list
        struct list_element *new_element = new struct list_element(key, value, table[bucket]);
        table[bucket] = new_element;

        return true;
    }

    // retrieves a value from the table
    std::optional<V> get(K key)
    {
        size_t bucket = hash(key);
        // obtain read lock
        std::shared_lock lock(*(locks[bucket]));

        // search for key in bucket
        struct list_element *cur = table[bucket];
        while (cur)
        {
            if (cur->key == key)
            {
                return cur->value;
            }
            cur = cur->next;
        }

        // key not found, return empty optional
        return {};
    }

    // removes a key value pair from the table
    // returns true iff key was found and removed
    bool remove(K key)
    {
        size_t bucket = hash(key);
        // obtain write lock
        std::unique_lock lock(*(locks[bucket]));

        // save parent linked list node for removal
        struct list_element *cur = table[bucket];
        struct list_element *pre = nullptr;

        // search bucket for key
        while (cur)
        {
            if (cur->key == key)
            {
                // key was found
                if (cur == table[bucket])
                {
                    // node has no parent
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

        // key was not found
        return false;
    }

    ~Hashtable()
    {
        // iterate over buckets
        for (struct list_element *cur : table)
        {
            // destruct linked list nodes
            while (cur)
            {
                struct list_element *tmp = cur;
                cur = cur->next;
                delete tmp;
            }
        }
    }
};
