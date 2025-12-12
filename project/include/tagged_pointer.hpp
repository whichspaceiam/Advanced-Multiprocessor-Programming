#pragma once
#include <atomic>


template <typename T>
class TaggedPointer
{
private:
    std::atomic<uintptr_t> data;

    static constexpr uintptr_t MARK_MASK = 0x1;   
    static constexpr uintptr_t VERSION_BITS = 16;
    static constexpr uintptr_t VERSION_MASK = ((1ULL << VERSION_BITS) - 1) << 1;
    static constexpr uintptr_t PTR_MASK = ~((1ULL << (VERSION_BITS + 1)) - 1);

    static uintptr_t pack(T *ptr, uint16_t version, bool mark)
    {
        return (reinterpret_cast<uintptr_t>(ptr) & PTR_MASK) |
               ((static_cast<uintptr_t>(version) << 1) & VERSION_MASK) |
               (mark ? 1 : 0);
    }

    static T *extractPointer(uintptr_t val)
    {
        return reinterpret_cast<T *>(val & PTR_MASK);
    }

    static uint16_t extractVersion(uintptr_t val)
    {
        return (val & VERSION_MASK) >> 1;
    }

    static bool extractMark(uintptr_t val)
    {
        return (val & MARK_MASK) != 0;
    }

public:
    TaggedPointer(T *ptr = nullptr, uint16_t version = 0, bool mark = false)
    {
        data.store(pack(ptr, version, mark), std::memory_order_relaxed);
    }

    TaggedPointer(TaggedPointer &&other) noexcept
    {
        data.store(other.data.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }

    TaggedPointer &operator=(TaggedPointer &&other) noexcept
    {
        data.store(other.data.load(std::memory_order_relaxed), std::memory_order_relaxed);
        return *this;
    }

    ~TaggedPointer() = default;
    T *getPointer() const
    {
        return extractPointer(data.load(std::memory_order_relaxed));
    }

    void get(T **outPtr, uint16_t *outVersion, bool *outMark) const
    {
        uintptr_t val = data.load(std::memory_order_relaxed);
        *outPtr = extractPointer(val);
        *outVersion = extractVersion(val);
        *outMark = extractMark(val);
    }

    bool compareAndSet(T *expectedPtr, uint16_t expectedVer, bool expectedMark,
                       T *newPtr, uint16_t newVer, bool newMark)
    {
        uintptr_t expected = pack(expectedPtr, expectedVer, expectedMark);
        uintptr_t desired = pack(newPtr, newVer, newMark);
        return data.compare_exchange_strong(expected, desired,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire);
    }
};