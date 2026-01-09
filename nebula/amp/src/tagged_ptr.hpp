template <typename T>
class TaggedPointer
{
private:
    std::atomic<uintptr_t> data;

    static constexpr uintptr_t PTR_MASK = 0x0000FFFFFFFFFFFF;
    static constexpr uintptr_t VERSION_MASK = 0xFFFF000000000000;
    static constexpr int VERSION_SHIFT = 48;

    static_assert(sizeof(void *) == 8, "64-bit platform required");

    static uintptr_t pack(T *ptr, uint16_t version)
    {
        uintptr_t ptrVal = reinterpret_cast<uintptr_t>(ptr);
        assert((ptrVal & PTR_MASK) == ptrVal && "Pointer exceeds 48-bit space");
        return (ptrVal & PTR_MASK) | (static_cast<uintptr_t>(version) << VERSION_SHIFT);
    }

    static T *extractPointer(uintptr_t val)
    {
        return reinterpret_cast<T *>(val & PTR_MASK);
    }

    static uint16_t extractVersion(uintptr_t val)
    {
        return static_cast<uint16_t>((val & VERSION_MASK) >> VERSION_SHIFT);
    }

public:
    TaggedPointer(T *ptr = nullptr, uint16_t version = 0)
        : data(pack(ptr, version)) {}

    TaggedPointer(const TaggedPointer &other)
        : data(other.data.load(std::memory_order_acquire)) {}

    TaggedPointer &operator=(const TaggedPointer &other)
    {
        if (this != &other)
        {
            data.store(other.data.load(std::memory_order_acquire),
                       std::memory_order_release);
        }
        return *this;
    }

    TaggedPointer(TaggedPointer &&other) noexcept
        : data(other.data.exchange(pack(nullptr, 0), std::memory_order_acq_rel)) {}

    TaggedPointer &operator=(TaggedPointer &&other) noexcept
    {
        if (this != &other)
        {
            data.store(other.data.exchange(pack(nullptr, 0), std::memory_order_acq_rel),
                       std::memory_order_release);
        }
        return *this;
    }

    ~TaggedPointer() = default;

    T *getPointer(std::memory_order order = std::memory_order_acquire) const
    {
        return extractPointer(data.load(order));
    }

    uint16_t getVersion(std::memory_order order = std::memory_order_acquire) const
    {
        return extractVersion(data.load(order));
    }

    std::pair<T *, uint16_t> load(std::memory_order order = std::memory_order_acquire) const
    {
        uintptr_t val = data.load(order);
        return {extractPointer(val), extractVersion(val)};
    }

    void store(T *ptr, uint16_t version,
               std::memory_order order = std::memory_order_release)
    {
        data.store(pack(ptr, version), order);
    }

    void get(T **outPtr, uint16_t *outVersion,
             std::memory_order order = std::memory_order_acquire) const
    {
        if (!outPtr || !outVersion)
        {
            return;
        }

        uintptr_t val = data.load(order);
        *outPtr = extractPointer(val);
        *outVersion = extractVersion(val);
    }

    bool compareAndSet(T *&expectedPtr, uint16_t &expectedVer,
                       T *newPtr, uint16_t newVer,
                       std::memory_order success = std::memory_order_release,
                       std::memory_order failure = std::memory_order_relaxed)
    {
        uintptr_t expected = pack(expectedPtr, expectedVer);
        uintptr_t desired = pack(newPtr, newVer);

        bool result = data.compare_exchange_strong(expected, desired, success, failure);

        if (!result)
        {
            // Update expected with actual value
            expectedPtr = extractPointer(expected);
            expectedVer = extractVersion(expected);
        }

        return result;
    }
};