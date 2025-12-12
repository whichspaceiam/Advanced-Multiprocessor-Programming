#include <cassert>
#include "sequential.hpp"   // your FreeList + Node
#include "structures.hpp"

using namespace seq;

static bs::Node* make(int v) {
    bs::Node* n = new bs::Node;
    n->value = v;
    n->next = nullptr;
    return n;
}

int main() {

    // ---------------------------------------------------------
    // 1. Empty list: get() must return nullptr
    // ---------------------------------------------------------
    {
        bs::FreeList fl;
        assert(fl.get(10) == nullptr);
    }

    // ---------------------------------------------------------
    // 2. push() one element, get() same value
    // ---------------------------------------------------------
    {
        bs::FreeList fl;
        fl.push(make(5));
        bs::Node* n = fl.get(5);
        assert(n != nullptr);
        assert(n->value == 5);
        delete n;   // returned node no longer owned by FreeList
    }

    // ---------------------------------------------------------
    // 3. push() multiple elements, get() head element
    // ---------------------------------------------------------
    {
        bs::FreeList fl;
        fl.push(make(1));
        fl.push(make(2));
        fl.push(make(3));    // header = 3

        bs::Node* n = fl.get(3);
        assert(n != nullptr);
        assert(n->value == 3);
        delete n;
    }

    // ---------------------------------------------------------
    // 4. push() multiple elements, get() middle element
    // ---------------------------------------------------------
    {
        bs::FreeList fl;
        fl.push(make(1)); // bottom
        fl.push(make(2));
        fl.push(make(3)); // top

        bs::Node* n = fl.get(2); // middle
        assert(n != nullptr);
        assert(n->value == 2);
        delete n;

        // remaining list: 3 -> 1
        assert(fl.get(3)->value == 3);
        assert(fl.get(1)->value == 1);
    }

    // ---------------------------------------------------------
    // 5. get non-existing value
    // ---------------------------------------------------------
    {
        bs::FreeList fl;
        fl.push(make(10));
        fl.push(make(20));

        bs::Node* n = fl.get(123456);
        assert(n == nullptr);
    }

    // ---------------------------------------------------------
    // 6. correct deletion by destructor (no crash / valgrind clean)
    // ---------------------------------------------------------
    {
        bs::FreeList fl;
        for (int i = 0; i < 100; i++)
            fl.push(make(i));
        // leaving scope must delete all nodes correctly
    }

    // ---------------------------------------------------------
    // 7. alternating push / get
    // ---------------------------------------------------------
    {
        bs::FreeList fl;
        fl.push(make(7));
        assert(fl.get(7)->value == 7);

        fl.push(make(8));
        fl.push(make(9));
        assert(fl.get(8)->value == 8);
        assert(fl.get(9)->value == 9);
    }

    // ---------------------------------------------------------
    // 8. ensure next pointers are updated correctly on middle removal
    // ---------------------------------------------------------
    {
        bs::FreeList fl;
        bs::Node* a = make(1);
        bs::Node* b = make(2);
        bs::Node* c = make(3);

        fl.push(a); // list: a
        fl.push(b); // list: b -> a
        fl.push(c); // list: c -> b -> a

        fl.get(2); // removes b

        bs::Node* n1 = fl.get(3); // head should be c
        bs::Node* n2 = fl.get(1); // then a

        assert(n1->value == 3);
        assert(n2->value == 1);

        delete n1;
        delete n2;
    }

    return 0;
}
