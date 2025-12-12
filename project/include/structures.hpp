#pragma once
#include <atomic>

namespace generics
{
    using value_t = int;
    constexpr value_t empty_val = -1000000;

}; // namespace generics

namespace interface
{
    class BaseQueue
    {

    public:
        using value_t = generics::value_t;

        virtual bool push(value_t v) = 0;
        virtual value_t pop() = 0;
        virtual int get_size() const = 0;
        virtual ~BaseQueue() = default;
    };
};

namespace bs // basic structures
{
    using value_t = generics::value_t;
    inline value_t empty_val = generics::empty_val;

    struct Node
    {
        Node *next;
        value_t value;
    };

    class FreeList
    {
        Node *header = nullptr;
        unsigned int size = 0;

    public:
        FreeList() = default;
        ~FreeList();
        FreeList(FreeList const &other);
        FreeList(FreeList &&other);
        FreeList &operator=(FreeList const &other);
        FreeList &operator=(FreeList &&other);

        void push(Node *n);
        Node *get(value_t val);
    };
};

namespace as // atomic structures
{
    using value_t = generics::value_t;

    template <typename T>
    class TaggedPointer
    {
    private:
        std::atomic<uintptr_t> data;

        static uintptr_t pack(T *ptr, uint16_t version, bool mark);
        static T *extractPointer(uintptr_t val);
        static uint16_t extractVersion(uintptr_t val);
        static bool extractMark(uintptr_t val);

    public:
        static constexpr uintptr_t MARK_MASK = 0x1;
        static constexpr uintptr_t VERSION_BITS = 16;
        static constexpr uintptr_t VERSION_MASK = ((1ULL << VERSION_BITS) - 1) << 1;
        static constexpr uintptr_t PTR_MASK = ~((1ULL << (VERSION_BITS + 1)) - 1);

        TaggedPointer(T *ptr = nullptr, uint16_t version = 0, bool mark = false);
        TaggedPointer(TaggedPointer &&other) noexcept;

        TaggedPointer &operator=(TaggedPointer &&other) noexcept;

        ~TaggedPointer() = default;
        T *getPointer() const;

        void get(T **outPtr, uint16_t *outVersion, bool *outMark) const;

        bool compareAndSet(T *expectedPtr, uint16_t expectedVer, bool expectedMark,
                           T *newPtr, uint16_t newVer, bool newMark);
    };

    struct alignas(1 << 17) Node
    {
        TaggedPointer<Node> next; // pointer + version + mark
        int value;
    };

    class FreeList
    {
        TaggedPointer<Node> header{};
        unsigned int size = 0;

    public:
        FreeList() = default;

        ~FreeList();

        FreeList(const FreeList &other);
        FreeList &operator=(const FreeList &other);
        FreeList(FreeList &&other) noexcept;
        FreeList &operator=(FreeList &&other) noexcept;
        void push(Node *n);
        Node *get(value_t val);
    };

};
