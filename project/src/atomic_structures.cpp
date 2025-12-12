#include "structures.hpp"
#include <utility>
template <typename T>
uintptr_t as::TaggedPointer<T>::pack(T *ptr, uint16_t version, bool mark)
{
    return (reinterpret_cast<uintptr_t>(ptr) & PTR_MASK) |
           ((static_cast<uintptr_t>(version) << 1) & VERSION_MASK) |
           (mark ? 1 : 0);
}

template <typename T>
T *as::TaggedPointer<T>::extractPointer(uintptr_t val)
{
    return reinterpret_cast<T *>(val & PTR_MASK);
}

template <typename T>
uint16_t as::TaggedPointer<T>::extractVersion(uintptr_t val)
{
    return (val & VERSION_MASK) >> 1;
}

template <typename T>
bool as::TaggedPointer<T>::extractMark(uintptr_t val)
{
    return (val & MARK_MASK) != 0;
}

template <typename T>
as::TaggedPointer<T>::TaggedPointer(T *ptr, uint16_t version, bool mark)
{
    data.store(pack(ptr, version, mark), std::memory_order_relaxed);
}

template <typename T>
as::TaggedPointer<T>::TaggedPointer(TaggedPointer &&other) noexcept
{
    data.store(other.data.load(std::memory_order_relaxed), std::memory_order_relaxed);
}

template <typename T>
as::TaggedPointer<T> &as::TaggedPointer<T>::operator=(TaggedPointer &&other) noexcept
{
    data.store(other.data.load(std::memory_order_relaxed), std::memory_order_relaxed);
    return *this;
}

template <typename T>
T *as::TaggedPointer<T>::getPointer() const
{
    return extractPointer(data.load(std::memory_order_relaxed));
}

template <typename T>
void as::TaggedPointer<T>::get(T **outPtr, uint16_t *outVersion, bool *outMark) const
{
    uintptr_t val = data.load(std::memory_order_relaxed);
    *outPtr = extractPointer(val);
    *outVersion = extractVersion(val);
    *outMark = extractMark(val);
}

template <typename T>
bool as::TaggedPointer<T>::compareAndSet(T *expectedPtr, uint16_t expectedVer, bool expectedMark,
                                         T *newPtr, uint16_t newVer, bool newMark)
{
    uintptr_t expected = pack(expectedPtr, expectedVer, expectedMark);
    uintptr_t desired = pack(newPtr, newVer, newMark);
    return data.compare_exchange_strong(expected, desired,
                                        std::memory_order_acq_rel,
                                        std::memory_order_acquire);
}


template class as::TaggedPointer<as::Node>;

// using namespace as;

as::FreeList::~FreeList()
{
    Node *walker = header.getPointer();
    while (walker)
    {
        Node *current = walker;
        walker = walker->next.getPointer();
        delete current;
    }
}

as::FreeList::FreeList(const FreeList &other)
{
    Node *otherWalker = other.header.getPointer();
    if (!otherWalker)
    {
        header = TaggedPointer<Node>(nullptr);
        return;
    }

    // Copy first node
    Node *newHead = new Node;
    newHead->value = otherWalker->value;
    header = TaggedPointer<Node>(newHead);

    Node *curThis = newHead;
    otherWalker = otherWalker->next.getPointer();

    while (otherWalker)
    {
        Node *n = new Node;
        n->value = otherWalker->value;
        n->next = TaggedPointer<Node>(nullptr);

        curThis->next = TaggedPointer<Node>(n);
        curThis = n;
        otherWalker = otherWalker->next.getPointer();
    }
}

// Copy assignment (deep copy)
as::FreeList &as::FreeList::operator=(const FreeList &other)
{
    if (this == &other)
        return *this;

    // Delete current list
    Node *walker = header.getPointer();
    while (walker)
    {
        Node *current = walker;
        walker = walker->next.getPointer();
        delete current;
    }

    Node *otherWalker = other.header.getPointer();
    if (!otherWalker)
    {
        header = TaggedPointer<Node>(nullptr);
        return *this;
    }

    Node *newHead = new Node;
    newHead->value = otherWalker->value;
    header = TaggedPointer<Node>(newHead);

    Node *curThis = newHead;
    otherWalker = otherWalker->next.getPointer();

    while (otherWalker)
    {
        Node *n = new Node;
        n->value = otherWalker->value;
        n->next = TaggedPointer<Node>(nullptr);

        curThis->next = TaggedPointer<Node>(n);
        curThis = n;
        otherWalker = otherWalker->next.getPointer();
    }

    return *this;
}

// Move constructor
as::FreeList::FreeList(FreeList &&other) noexcept
{
    header = std::move(other.header);
    other.header = TaggedPointer<Node>(nullptr);
}

// Move assignment
as::FreeList &as::FreeList::operator=(FreeList &&other) noexcept
{
    if (this != &other)
    {
        // Delete current
        Node *walker = header.getPointer();
        while (walker)
        {
            Node *current = walker;
            walker = walker->next.getPointer();
            delete current;
        }

        header = std::move(other.header);
        other.header = TaggedPointer<Node>(nullptr);
    }
    return *this;
}

void as::FreeList::push(Node *n)
{
    n->next = std::move(header);
    header = TaggedPointer<Node>(n);
    size++;
}

as::Node *as::FreeList::get(value_t val)
{
    Node *walker = header.getPointer();
    if (!walker)
        return nullptr;

    // Check head first
    if (walker->value == val)
    {
        Node *toRtn = walker;
        header = std::move(walker->next);
        size--;
        return toRtn;
    }

    while (walker->next.getPointer())
    {
        Node *nextPtr = walker->next.getPointer();
        if (nextPtr->value == val)
        {
            walker->next = std::move(nextPtr->next);
            size--;
            return nextPtr;
        }
        walker = nextPtr;
    }

    return nullptr;
}
