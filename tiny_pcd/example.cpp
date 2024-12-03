#include "tiny_pcd.h"

#include <cstdio>
#include <string>

struct Point {
  float x;
  float y;
  float z;
  uint32_t intensity;
};

int main(int argc, char *argv[]) {
  const std::string pcd_file = argv[1];

  tiny_pcd::TinyPcd pcd(pcd_file);
  const auto header = pcd.header();

  printf("Version: %s\n", header.version.c_str());
  printf("Width: %u\n", header.width);
  printf("Height: %u\n", header.height);
  printf("Points: %lu\n", header.points);
  for (int i = 0; i < header.field.size(); ++i) {
    printf("Field: %s, Size: %d, Type: %s, Count: %d\n", header.field[i].c_str(), header.size[i],
           header.type[i].c_str(), header.count[i]);
  }

  for (const auto &point : pcd) {
    Point p{
        .x = point.get<float>("x"),
        .y = point.get<float>("y"),
        .z = point.get<float>("z"),
        .intensity = point.get<uint32_t>("intensity"),
    };
    printf("Point: (%f, %f, %f), Intensity: %d\n", p.x, p.y, p.z, p.intensity);
  }

  return 0;
}