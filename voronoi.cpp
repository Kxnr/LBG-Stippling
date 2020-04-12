#include "voronoi.h"

std::vector<VoronoiCell> GetVoronoiCells(const CImg<short>& img, const std::vector<vec3>& pts) {
  // moments
  std::vector<VoronoiCell> voronoi = std::vector<VoronoiCell>(pts.size());
  
  float dist, minDist, density;
  float fx, fy; // normalized coordinates
  cimg_forXY(img, x, y) {

    fx = x / (float) img.width();
    fy = y / (float) img.height();

    VoronoiCell* cell;
    minDist = std::numeric_limits<float>::infinity();
    dist = 0;
    for(int i : range(pts.size())) {
      dist = (glm(fx, fy, 0) - pts[i]).length();
      if(dist < minDist) {
        minDist = dist;
        cell = &voronoi[i];
      }
    }

    cell->area++;

    density = 255.0f -  img(x,y,0,0);

    cell->density += density / 255.0f;
    cell->m00 += density;

    cell->m10 += x * density;
    cell->m01 += y * density;
    cell->m11 += x * y * density;

    cell->m20 += x * x * density;
    cell->m02 += y * y * density;
  }

  float a, b, c, angle;
  float x, y, magnitude;

  for(int i : range(voronoi.size())) {
    voronoi[i].centroid[0] = voronoi[i].m10 / (float) voronoi[i].m00;
    voronoi[i].centroid[1] = voronoi[i].m01 / (float) voronoi[i].m00;

    a = voronoi[i].m20 / voronoi[i].m00 - voronoi[i].centroid.x() * voronoi[i].centroid.x();
    b = 2.0f * (voronoi[i].m11 / voronoi[i].m00 - voronoi[i].centroid.x() * voronoi[i].centroid.y());
    c = voronoi[i].m02 / voronoi[i].m00 - voronoi[i].centroid.y() * voronoi[i].centroid.y();
    angle = std::atan2(b, a - c) / 2.0f;

    voronoi[i].centroid[0] = (float) img.width();
    voronoi[i].centroid[1] = (float) img.height();

    voronoi[i].centroid[0] = std::clamp(voronoi[i].centroid[0], 0, 1.0f); 
    voronoi[i].centroid[1] = std::clamp(voronoi[i].centroid[1], 0, 1.0f); 
    

    magnitude = std::max(1.0f, voronoi[i].area); 
    magnitude /= PI;
    magnitude = std::sqrt(magnitude);
    magnitude /= 2.0f;

    // bootleg rotation matrix
    x = std::cos(angle) / img.width(); 
    y = std::sin(angle) / img.height();

    voronoi[i].axis = vec3(x, y, 0) * magnitude;
  }

  return voronoi;
}

// CImg<short> DrawVoronoiMap(const std::vector<VoronoiCell> cells) {
//   // shade voronoi cells with gray value, to get a stipple like effect
//   // using facets rather than stipples
//
// }
