#ifndef VORONOI_H
#define VORONOI_H

#include <vector>
#define cimg_use_jpeg
#include "CImg.h"
#include "utils.h"
#include "glm/glm.hpp"

using namespace cimg_library;

#define PI 3.1415926536

struct VoronoiCell {
  glm::vec2 centroid {0.0f, 0.0f};
  float angle = 0;
  float area = 0;
  float m00 = 0;
  float m01 = 0;
  float m10 = 0;
  float m11 = 0;
  float m20 = 0;
  float m02 = 0;
};

std::vector<VoronoiCell> GetVoronoiCells(const CImg<unsigned char>& map, const CImg<unsigned char>& img, std::vector<glm::vec2> pts);

#endif
