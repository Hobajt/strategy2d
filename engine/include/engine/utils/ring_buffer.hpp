#pragma once 

namespace eng {

    //Dirty impl, doesn't destroy objects properly, don't use on objects that do custom allocations.
    template <typename T> 
    class RingBuffer {
    public:
        RingBuffer() : RingBuffer(8) {}
        RingBuffer(int capacity);
        ~RingBuffer();

        RingBuffer(const RingBuffer&) = delete;
        RingBuffer& operator=(const RingBuffer&) = delete;

        RingBuffer(RingBuffer&&) noexcept;
        RingBuffer& operator=(RingBuffer&&) noexcept;

        void Push(const T& entry);
        void Push(T&& entry);
        T Pop();

        void Clear();

        int Size() const { return size; }
    private:
        T* buf;
        int capacity;
        int head;
        int size;
    };

    //=================================

    template <typename T> inline RingBuffer<T>::RingBuffer(int capacity_) : capacity(capacity_), size(0), head(0) {
        buf = new T[capacity];
    }

    template <typename T> inline RingBuffer<T>::~RingBuffer() {
        delete[] buf;
    }

    template <typename T> inline RingBuffer<T>::RingBuffer(RingBuffer&& b) noexcept {
        buf = b.buf;
        capacity = b.capacity;
        size = b.size;
        head = b.head;
        b.buf = nullptr;
    }

    template <typename T> inline RingBuffer<T>& RingBuffer<T>::operator=(RingBuffer&& b) noexcept {
        if(buf != nullptr)
            delete[] buf;
        buf = b.buf;
        capacity = b.capacity;
        size = b.size;
        head = b.head;
        b.buf = nullptr;
    }

    template <typename T> inline void RingBuffer<T>::Push(const T& entry) {
        buf[head] = entry;
        head = (head+1) % capacity;
        if((++size) > capacity)
            size = capacity;
    }

    template <typename T> inline void RingBuffer<T>::Push(T&& entry) {
        buf[head] = std::move(entry);
        head = (head+1) % capacity;
        if((++size) > capacity)
            size = capacity;
    }

    template <typename T> inline T RingBuffer<T>::Pop() {
        //TODO: call object destructors on pop
        if(size > 0) {
            head = (head-1+capacity) % capacity;
            size--;
            
            return buf[head];
        }
        throw std::out_of_range("RingBuffer is empty.");
    }

    template <typename T> inline void RingBuffer<T>:: Clear() {
        size = 0;
        head = 0;
    }

}//namespace eng
