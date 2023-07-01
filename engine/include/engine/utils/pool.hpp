#pragma once

#define POOL_STARTING_SIZE 1

#include <stdexcept>
#include <exception>
#include <cstddef>
#include <memory>

#define POOL_NO_HOLE_SORTING

namespace eng {

    //Custom data structure for object pooling - essentially a list + stack that tracks empty places.
    //Insert & remove are O(1), but element order isn't respected (elements are inserted into empty places, not at the end of struct).
    //Element adressing & access:
    //  - objects are accessible either through slot index, object's identifier or key, which combines both values
    //  - identifiers = essentially a key (std::map) - specified when inserting & used for it's addressing; should be unique (but doesn't have to be)
    //Insert can cause reallocation, so in general, it doesn't preserve pointers.
    //Iteration:
    //  - not ideal, need to iterate the entire pool, including holes
    //  - complexity is dependent on capacity, not current size
    //  - is still faster than maps (with reasonable size/capacity ratio)
    template <typename T, typename K = size_t>
    class pool {
    public:
        using idxType = size_t;
        using keyType = K;

        //Identifies object within the pool (array index + object identifier).
        struct key {
            idxType idx;
            keyType id;
        public:
            key() : idx((idxType)-1), id(keyType()) {}
            key(idxType idx_) : idx(idx_), id(keyType()) {}
            key(idxType idx_, keyType id_) : idx(idx_), id(id_) {}
        };

        //Contents of a single slot in the pool (data + metadata).
        struct slot {
            T data;
            key info;
            bool taken;
        };
    public:
        struct iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = T;
            using pointer = slot*;
            using reference = T&;
        public:
            iterator(pointer ptr, pointer end) : m_ptr(ptr), m_end_ptr(end) {}

            reference operator*() const { return m_ptr->data; }
            pointer operator->() { return m_ptr; }
            
            iterator& operator++() { 
                m_ptr++;
                while (m_ptr < m_end_ptr && !m_ptr->taken)
                    m_ptr++;
                return *this; 
            }
            iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }

            friend bool operator== (const iterator& a, const iterator& b) { return a.m_ptr == b.m_ptr; }
            friend bool operator!= (const iterator& a, const iterator& b) { return a.m_ptr != b.m_ptr; }
        private:
            pointer m_ptr;
            pointer m_end_ptr;
        };

        struct const_iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = T;
            using pointer = slot const*;
            using reference = T const&;
        public:
            const_iterator(pointer ptr, pointer end) : m_ptr(ptr), m_end_ptr(end) {}
            const_iterator(const iterator& it)  : m_ptr(it.ptr), m_end_ptr(it.end) {}

            reference operator*() const { return m_ptr->data; }
            pointer operator->() { return m_ptr; }
            
            const_iterator& operator++() { 
                m_ptr++;
                while (m_ptr < m_end_ptr && !m_ptr->taken)
                    m_ptr++;
                return *this; 
            }
            const_iterator operator++(int) { const_iterator tmp = *this; ++(*this); return tmp; }

            friend bool operator== (const const_iterator& a, const const_iterator& b) { return a.m_ptr == b.m_ptr; }
            friend bool operator!= (const const_iterator& a, const const_iterator& b) { return a.m_ptr != b.m_ptr; }
        private:
            pointer m_ptr;
            pointer m_end_ptr;
        };

        iterator begin() { return iterator(&m_data[m_first], &m_data[m_size+m_holes]); }
        iterator end() { return iterator(&m_data[m_size+m_holes], &m_data[m_size+m_holes]); }

        const_iterator begin() const { return const_iterator(&m_data[m_first], &m_data[m_size+m_holes]); }
        const_iterator end() const { return const_iterator(&m_data[m_size+m_holes], &m_data[m_size+m_holes]); }
    public:
        pool();
        pool(idxType capacity);
        ~pool();

        //copy enabled
        pool(const pool&) noexcept;
        pool& operator=(const pool&) noexcept;

        //move enabled
        pool(pool&&) noexcept;
        pool& operator=(pool&&) noexcept;

        //========

        idxType size() const { return m_size; }
        idxType capacity() const { return m_capacity; }

        void clear();

        //Checks whether object with given key is stored in the pool.
        bool exists(const key& k) const;

        //Checks whether given slot is taken or not.
        bool taken(idxType idx) const;

        //Element access using slot index (throws on miss).
        T& operator[](idxType idx);
        const T& operator[](idxType idx) const;

        //Element access using key (throws on miss).
        T& operator[](const key& k);
        const T& operator[](const key& k) const;
        
        //Lookup index of an object with specific identifier. Returns -1 when not found. Iterates through the entire pool - O(n).
        idxType find(const keyType& id);

        //========

        //Adds given element into the pool & returns it's key.
        key add(const keyType& id, const T& element);
        key add(const keyType& id, T&& element);

        //Add with in-place object creation.
        template <typename... Args>
        key emplace(const keyType& id, Args&&... args);

        //Removes element under given key (or in given slot).
        bool remove(idxType idx);
        bool remove(const key& k);

        //Removes element under given key (or in given slot) and returns it. Throws on miss.
        T withdraw(idxType idx);
        T withdraw(const key& k);

        //withdraw() version that doesn't throw, result is returned through argument instead.
        bool withdraw(idxType idx, T& out_data);
        bool withdraw(const key& k, T& out_data);
    private:
        void allocate(idxType capacity);
        void extend();

        idxType fetchFreeSlot(const keyType& id);
        void clearSlot(idxType idx);
        void sortHoles();

        void copy(const pool& p) noexcept;
        void move(pool&& p) noexcept;
        void release() noexcept;
    private:
        slot* m_data;
        idxType m_capacity;
        idxType m_size;
        idxType m_holes;
        idxType m_first;
    };

}//namespace eng

//========================

#include "pool.inc"

#undef POOL_STARTING_SIZE
#undef POOL_NO_HOLE_SORTING
