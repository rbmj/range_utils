Range Utils
===========

This repository contains a couple of header-only libraries to provide
some simple range manipulations.  For example:

```c++
#include <iostream>
#include <ranges>
#include "zip_range.h"
#include "xform_range.h"


int main() {
    int x[] = {1, 2, 3, 4, 5};
    float f[] = {1.1, 1.2, 1.3, 1.4, 1.5};
    auto z = zip(x, xform(x, [](int y) { return y*y; }), f);
    for (auto i : z) {
        //works for output iterators too - this modifies x
        std::get<0>(i) *= 2;
        std::cout << std::get<0>(i) << " " << std::get<1>(i) 
            << " " << std::get<2>(i) << "\n";
    }
    /* Output:
     * 2 1 1.1
     * 4 4 1.2
     * 6 9 1.3
     * 8 16 1.4
     * 10 25 1.5
     */
    return 0;
}
```

