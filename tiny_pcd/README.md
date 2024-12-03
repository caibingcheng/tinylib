### tiny_pcd

A library for just reading and writing PCD files. It's simple and wrote in C++17 within 200 lines.

### usage

1. Include the header file `tiny_pcd.h` in your project.
2. Use the namespace `tiny_pcd`.
3. Create a `TinyPCD` object.
4. Read points by `for` loop.

```cpp
#include "tiny_pcd.h"

struct Point {
  float x, y, z;
};

int main() {
  using namespace tiny_pcd;

  TinyPcd pcd("test.pcd");

  for (const auto &point : pcd) {
    const Point p{
        .x = point.get<float>("x"),
        .y = point.get<float>("y"),
        .z = point.get<float>("z"),
    };

    printf("x: %f, y: %f, z: %f\n", p.x, p.y, p.z);
  }
}
```
