#include <iostream>
#include <ranges>
#include "zip_range.h"

int main() {
    int x[] = {1, 2, 3, 4, 5};
    int y[] = {2, 4, 6, 8, 10};
    float f[] = {1.1, 1.2, 1.3, 1.4, 1.5};
    auto z = zip(x, y, f);
    for (auto i : z) {
        std::get<0>(i) *= 2;
        std::cout << std::get<0>(i) << " " << std::get<1>(i) 
            << " " << std::get<2>(i) << "\n";
    }
    for (auto i : x) {
        std::cout << i << '\n';
    }
    return 0;
}
