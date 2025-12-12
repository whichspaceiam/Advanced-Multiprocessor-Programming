#include <iostream>
#include <climits>

int main()
{
    std::cout << "char: " << CHAR_BIT << " bits\n";
    std::cout << "short: " << sizeof(short) * CHAR_BIT << " bits\n";
    std::cout << "int: " << sizeof(int) * CHAR_BIT << " bits\n";
    std::cout << "long: " << sizeof(long) * CHAR_BIT << " bits\n";
    std::cout << "long long: " << sizeof(long long) * CHAR_BIT << " bits\n";
    std::cout << "float: " << sizeof(float) * CHAR_BIT << " bits\n";
    std::cout << "double: " << sizeof(double) * CHAR_BIT << " bits\n";
    return 0;
}
