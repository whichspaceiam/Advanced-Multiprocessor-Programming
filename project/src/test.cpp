#include <cstdint>
#include <iostream>

int main() {
    int x = 5;

    int* x_ptr = &x;

    // Safely convert to integer
    uintptr_t ptr_val = reinterpret_cast<uintptr_t>(x_ptr);

    // Example: mask lower 16 bits
    uintptr_t mask = ~static_cast<uintptr_t>(0xFFFF);

    uintptr_t masked_val = ptr_val & mask;

    // Convert back (result will likely be invalid!)
    int* masked_ptr = reinterpret_cast<int*>(masked_val);

    std::cout << std::hex << ptr_val << "\n";
    std::cout << std::hex << masked_val << "\n";
}
