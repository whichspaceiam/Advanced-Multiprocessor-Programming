#include "timer.hpp"
#include <iostream>
#include <cstdlib>

#define CHECK(cond)                                                             \
    do {                                                                        \
        if (!(cond)) {                                                          \
            std::cerr << "CHECK failed: " #cond                                 \
                      << " at " << __FILE__ << ":" << __LINE__ << "\n";         \
            return EXIT_FAILURE;                                                \
        }                                                                       \
    } while (0)

int main() {
    {
        Timer t;
        t.checkpoint();

        auto laps = t.get_laps<>();

        CHECK(laps.size() == 1);
        CHECK(laps[0] >= 0.0);
    }

    {
        Timer t;
        t.checkpoint();
        t.checkpoint();
        t.checkpoint();

        auto laps = t.get_laps<>();

        CHECK(laps.size() == 3);
    }

    return EXIT_SUCCESS;
}
