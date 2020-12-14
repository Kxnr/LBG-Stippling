#include "voronoi.h"

std::vector<VoronoiCell> GetVoronoiCells(const CImg<unsigned char>& map, const CImg<unsigned char>& img, const std::vector<glm::vec2> pts) {
    std::vector<VoronoiCell> voronoi(pts.size());

    float density;
    VoronoiCell* cell;

    cimg_library::CImg<unsigned char> error = cimg_library::CImg<unsigned char>(map.width(), map.height(), 1, 3);

    cimg_forXY(map, _x, _y) {
            uint32_t index = DecodeColor(map(_x,_y,0), map(_x,_y,1), map(_x,_y,2));

            cell = &voronoi[index];
            density = std::max(1.0f - img(_x, _y) / 255.0f, std::numeric_limits<float>::epsilon());

            cell->area++;
            cell->m00 += density;

            cell->m10 += _x * density;
            cell->m01 += _y * density;
            cell->m11 += _x * _y * density;

            cell->m20 += _x * _x * density;
            cell->m02 += _y * _y * density;
    }

    float a, b, c;

    for(int i : range(voronoi.size())) {

        // this cell will be removed
        if (voronoi[i].m00 <= 0.0f) continue;

        voronoi[i].centroid[0] = voronoi[i].m10 / voronoi[i].m00;
        voronoi[i].centroid[1] = voronoi[i].m01 / voronoi[i].m00;

        a = voronoi[i].m20 / voronoi[i].m00 - voronoi[i].centroid.x * voronoi[i].centroid.x;
        b = 2.0f * (voronoi[i].m11 / voronoi[i].m00 - voronoi[i].centroid.x * voronoi[i].centroid.y);
        c = voronoi[i].m02 / voronoi[i].m00 - voronoi[i].centroid.y * voronoi[i].centroid.y;
        voronoi[i].angle = std::atan2(b, a - c) / 2.0f;

        voronoi[i].centroid[0] = (voronoi[i].centroid[0] + .5f) / (float) img.width();
        voronoi[i].centroid[1] = (voronoi[i].centroid[1] + .5f) / (float) img.height();
    }

    return voronoi;
}