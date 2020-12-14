#ifndef UTILS_H
#define UTILS_H

#define PI 3.1415926536

#include <vector>
#include "glm/glm.hpp"
#include "glm/vec3.hpp"
#include <iostream>

// #define DEBUG

inline std::vector<int> range(int limit) {
  // TODO: tighten this up
  std::vector<int> V;
  V.reserve(limit);

  for(int i=0; i < limit; i++) {
    V.push_back(i);
  }

  return V;
}

inline glm::vec3 EncodeColor(uint32_t i) {
  uint8_t r = (i >> 16) & 0x000000ff;
  uint8_t g = (i >> 8) & 0x000000ff;
  uint8_t b = (i >> 0) & 0x000000ff;

#ifdef DEBUG
  if(i == 0) {
    return {1, 0, 0};
  }
  if(i == 1) {
    return {0, 1, 0};
  }
  if(i == 2) {
    return {0, 0, 1};
  }
#endif

  return {r / 255.0f, g / 255.0f, b / 255.0f};
}

inline uint32_t DecodeColor(unsigned char r, unsigned char g, unsigned char b) {
  uint32_t result = 0x00000000 | (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | (static_cast<uint32_t>(b));
  // std::cout << "decoded" << std::endl;
  // std::cout << std::hex << result  << " " << (int)r << " " << (int)g << " " << (int)b << std::endl;
  return result;
}


#endif
