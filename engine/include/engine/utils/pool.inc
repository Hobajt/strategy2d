//Method definitions for class template pool (header file pool.hpp)

#define POOL_CHECK_IDX_RANGE(idx) if((idx) >= m_capacity) { throw std::out_of_range("Index into the pool is out of range."); }
#define POOL_UNUSED_SLOT_EXCEPTION std::invalid_argument("Provided index maps to an unused slot in the pool.")
#define POOL_KEY_NOT_FOUND_EXCEPTION std::invalid_argument("Object with specified key isn't located in the pool.")

//============================

namespace eng {

    //===== Constructors =====

    template <typename T, typename K>
    pool<T, K>::pool() : m_capacity(0), m_size(0), m_holes(0), m_data(nullptr) {}

    template <typename T, typename K>
    pool<T, K>::pool(idxType capacity) : m_capacity(capacity), m_size(0), m_holes(0), m_data(nullptr) {
        allocate(capacity);
    }

    template <typename T, typename K>
    pool<T, K>::~pool() {
        release();
    }

    template <typename T, typename K>
    pool<T, K>::pool(const pool<T,K>& p) noexcept {
        copy(p);
    }

    template <typename T, typename K>
    pool<T,K>& pool<T,K>::operator=(const pool<T,K>& p) noexcept {
        release();
        copy(p);
        return *this;
    }

    template <typename T, typename K>
    pool<T, K>::pool(pool<T, K>&& p) noexcept {
        move(std::move(p));
    }

    template <typename T, typename K>
    pool<T, K>& pool<T, K>::operator=(pool<T, K>&& p) noexcept {
        release();
        move(std::move(p));
        return *this;
    }

    //===== Management/Access  =====

    template <typename T, typename K>
    void pool<T, K>::clear() {
        if (m_data != nullptr) {
            for (idxType i = 0; i < m_capacity; i++) {
                if (!m_data[i].taken)
                    m_data[i].data.~T();
                m_data[i].info = key(i);
                m_data[i].taken = false;
            }
        }
        m_size = 0;
    }

    template <typename T, typename K>
    bool pool<T, K>::exists(const key& k) const {
        POOL_CHECK_IDX_RANGE(k.idx);
        return m_data[k.idx].info.id == k.id;
    }

    template <typename T, typename K>
    bool pool<T, K>::taken(idxType idx) const {
        POOL_CHECK_IDX_RANGE(idx);
        return m_data[idx].taken;
    }

    template <typename T, typename K>
    T& pool<T, K>::operator[](idxType idx) {
        if (!taken(idx))
            throw POOL_UNUSED_SLOT_EXCEPTION;
        return m_data[idx].data;
    }

    template <typename T, typename K>
    const T& pool<T, K>::operator[](idxType idx) const {
        if (!taken(idx))
            throw POOL_UNUSED_SLOT_EXCEPTION;
        return m_data[idx].data;
    }

    template <typename T, typename K>
    T& pool<T, K>::operator[](const key& k) {
        if (!exists(k))
            throw POOL_KEY_NOT_FOUND_EXCEPTION;
        return m_data[k.idx].data;
    }

    template <typename T, typename K>
    const T& pool<T, K>::operator[](const key& k) const {
        if (!exists(k))
            throw POOL_KEY_NOT_FOUND_EXCEPTION;
        return m_data[k.idx].data;
    }

    template <typename T, typename K>
    typename pool<T, K>::idxType pool<T, K>::find(const keyType& id) {
        for (idxType i = 0; i < m_capacity; i++) {
            if (m_data[i].info.id == id)
                return i;
        }
        return (idxType)-1;
    }

    //===== Insert/Remove =====

    template <typename T, typename K>
    typename pool<T, K>::key pool<T, K>::add(const keyType& id, const T& element) {
        idxType idx = fetchFreeSlot(id);
        new(&m_data[idx].data) T(std::move(element));
        return key{ idx, id };
    }

    template <typename T, typename K>
    typename pool<T, K>::key pool<T, K>::add(const keyType& id, T&& element) {
        idxType idx = fetchFreeSlot(id);
        new(&m_data[idx].data) T(std::move(element));
        return key{ idx, id };
    }

    template <typename T, typename K>
    template <typename... Args>
    typename pool<T, K>::key pool<T, K>::emplace(const keyType& id, Args&&... args) {
        idxType idx = fetchFreeSlot(id);
        new(&m_data[idx].data) T(std::forward<Args>(args)...);
        return key{ idx, id };
    }

    template <typename T, typename K> 
    bool pool<T, K>::remove(idxType idx) {
        if (taken(idx)) {
            clearSlot(idx);
            return true;
        }
        else
            return false;
    }

    template <typename T, typename K> 
    bool pool<T, K>::remove(const key& k) {
        if(exists(k)) {
            clearSlot(k.idx);
            return true;
        }
        else
            return false;
    }

    template <typename T, typename K>
    T pool<T, K>::withdraw(idxType idx) {
        T out_data = T();
        if (!withdraw(idx, out_data))
            throw POOL_UNUSED_SLOT_EXCEPTION;
        return out_data;
    }

    template <typename T, typename K>
    T pool<T, K>::withdraw(const key& k) {
        T out_data = T();
        if (!withdraw(k, out_data))
            throw POOL_KEY_NOT_FOUND_EXCEPTION;
        return out_data;
    }

    template <typename T, typename K>
    bool pool<T, K>::withdraw(idxType idx, T& out_data) {
        POOL_CHECK_IDX_RANGE(idx);
        if (taken(idx)) {
            out_data = std::move(m_data[idx].data);
            clearSlot(idx);
            return true;
        }
        else
            return false;
    }

    template <typename T, typename K>
    bool pool<T, K>::withdraw(const key& k, T& out_data) {
        POOL_CHECK_IDX_RANGE(k.idx);
        if (exists(k)) {
            out_data = std::move(m_data[k.idx].data);
            clearSlot(k.idx);
            return true;
        }
        else
            return false;
    }

    //===== Helper methods =====

    template <typename T, typename K>
    void pool<T, K>::allocate(idxType capacity) {
        release();

        m_capacity = capacity;
        m_data = (slot*)::operator new(m_capacity * sizeof(slot));
        for (idxType i = 0; i < m_capacity; i++) {
            m_data[i].info = key(i);
            m_data[i].taken = false;
        }
    }

    template <typename T, typename K>
    void pool<T, K>::extend() {
        idxType capacity_new = m_capacity * 2;

        //allocate new buffer and move stuff over
        slot* tmp = (slot*)::operator new(capacity_new * sizeof(slot));
        for (idxType i = 0; i < m_capacity; i++) {
            new(&tmp[i]) slot(std::move(m_data[i]));
        }

        //remove existing buffer & update capacity
        for (idxType i = 0; i < m_capacity; i++) {
            m_data[i].~slot();
        }
        ::operator delete(m_data, m_capacity * sizeof(slot));
        m_data = tmp;
        m_capacity = capacity_new;

        //update control data
        for (idxType i = 0; i < m_capacity - m_size; i++) {
            m_data[m_size + i].info = key(m_size + i);
            m_data[m_size + i].taken = false;
        }
    }

    template <typename T, typename K>
    typename pool<T, K>::idxType pool<T, K>::fetchFreeSlot(const keyType& id) {
        //container resize
        if (m_data == nullptr)
            allocate(POOL_STARTING_SIZE);
        else if (m_size >= m_capacity)
            extend();
        
        //lookup free slot idx (pop from stack)
        idxType idx = m_data[m_size].info.idx;
        m_data[m_size].info.idx = (idxType)-1;

        //store object's key & increment object counter
        m_data[idx].taken = true;
        m_data[idx].info.id = id;
        m_size++;
        m_holes = (m_holes > 0) ? (m_holes-1) : 0;
        m_first = std::min(m_first, idx);

        return idx;
    }

    template <typename T, typename K>
    void pool<T, K>::clearSlot(idxType idx) {
        //decrement object counter & unset slot key and data
        m_size--;
        m_data[idx].info.id = keyType();
        m_data[idx].taken = false;
        m_data[idx].data.~T();

        //push freed slot index onto the stack
        m_data[m_size].info.idx = idx;

        m_holes++;
    #ifndef POOL_NO_HOLE_SORTING
        sortHoles();
    #endif
        if(idx == m_first) {
            while((++m_first) < (m_size+m_holes)) {
                if(m_data[m_first].taken)
                    break;
            }
        }
    }

    template <typename T, typename K>
    void pool<T, K>::sortHoles() {
        //binary search to find fitting position
        idxType val = m_data[m_size].info.idx;
        idxType start = m_size;
        idxType end = m_size + m_holes;
        idxType pos = start;
        while(start < end) {
            pos = start + (end-start)/2;
            if(val < m_data[pos].info.idx)
                end = pos;
            else
                start = pos+1;
        }

        // printf("S: %d, P: %d\n", (int)m_size, (int)pos);
        
        //insert sort to move new hole to proper position
        for(idxType i = m_size; i < pos; i++) {
            idxType tmp = m_data[i+1].info.idx;
            m_data[i+1].info.idx = m_data[i].info.idx;
            m_data[i].info.idx = tmp;

            //reduce hole counter when hole is in it's original spot
            if(m_data[i].info.idx == i)
                m_holes = std::min(m_holes, i-m_size);
        }
        if(m_data[pos].info.idx == pos)
            m_holes = std::min(m_holes, pos-m_size);
    }

    template <typename T, typename K>
    void pool<T, K>::copy(const pool& p) noexcept {
        m_size = p.m_size;
        m_holes = p.m_holes;
        m_first = p.m_first;
        m_capacity = p.m_capacity;
        m_data = (slot*)::operator new(m_capacity * sizeof(slot));
        for (idxType i = 0; i < m_capacity; i++) m_data[i] = slot(p.m_data[i]);
    }

    template <typename T, typename K>
    void pool<T, K>::move(pool&& p) noexcept {
        m_capacity = p.m_capacity;
        m_size = p.m_size;
        m_data = p.m_data;
        m_holes = p.m_holes;
        m_first = p.m_first;

        p.m_data = nullptr;
        p.m_holes = p.m_first = p.m_size = p.m_capacity = 0;
    }

    template <typename T, typename K>
    void pool<T, K>::release() noexcept {
        if (m_data != nullptr) {
            for (idxType i = 0; i < m_capacity; i++) {
                if (m_data[i].info.id != K())
                    m_data[i].data.~T();
                m_data[i].info.~key();
            }
            ::operator delete(m_data, m_capacity * sizeof(slot));
        }
        m_holes = m_first = m_size = m_capacity = 0;
    }

}//namespace eng 

//============================

#undef POOL_CHECK_IDX_RANGE
#undef POOL_UNUSED_SLOT_EXCEPTION
#undef POOL_KEY_NOT_FOUND_EXCEPTION
