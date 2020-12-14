#ifndef STIPPLES_H
#define STIPPLES_H

#include <vector>
#include <algorithm>
#include <random>
#include "CImg.h"
#include "voronoi.h"
#include "utils.h"
#include "gpuVoronoi.h"
#include "glm/glm.hpp"
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"
#include <glm/gtc/type_ptr.hpp>

using namespace cimg_library;

struct Params {
    int count;
    float jitter;
    float hStep;
    float hConst;
    float pointSize;
    int maxIterations;
    int maxPoints;
    glm::vec3 bgdColor;
    float multiplier;

    Params(int _count, float _jitter, float _hStep, float _hConst, float _pointSize, int _maxIters, int _maxPts, glm::vec3 _bgdColor, int _multiplier ) : count(_count), jitter(_jitter), hStep(_hStep), hConst(_hConst), pointSize(_pointSize), maxIterations(_maxIters), maxPoints(_maxPts), bgdColor(_bgdColor), multiplier(_multiplier) {}
};

struct Point {
    glm::vec2 pos;
    float size;
    glm::vec3 color;

    Point(glm::vec2 _pos, float _size, glm::vec3 _color) : pos(_pos), size(_size), color(_color) {}
};


inline std::vector<glm::vec2> GetCenters(std::vector<Point> pts) {
    std::vector<glm::vec2> centers;
    centers.reserve(pts.size());

    for(Point pt : pts) {
        centers.push_back(pt.pos);
    }

    return centers;
}


class StippleImage {
public:
    StippleImage(const CImg<unsigned char>& _img, const Params& _params);
    ~StippleImage() {
        delete voronoiSolver;
    }

    std::default_random_engine generator;

    void Iterate(float hysteresis);
    bool Solve();
    bool IsDone() const;
    bool IsError();

    CImg<unsigned char> DrawImage();

    std::vector<Point>* GetPoints();

private:
    const Params params;
    CImg<unsigned char> img;
    int changes = -1; // track splits and merges
    int iterations = 0;
    std::vector<Point> stipples;

    GPUVoronoi* voronoiSolver;

    glm::vec2 Jitter(glm::vec2 pt);
    inline float GetHysteresis() {return this->params.hConst
                                         + this->iterations * this->params.hStep;
    };

    inline float GetUpperSplitBound(float pointSize, float hysteresis) {
        return (1.0f + hysteresis / 2.0f) * PI * pointSize * pointSize * this->params.multiplier;
    }

    inline float GetLowerSplitBound(float pointSize, float hysteresis) {
        return (1.0f - hysteresis / 2.0f) * PI * pointSize * pointSize * this->params.multiplier;
    }

    std::vector<Point> GetRandomStipples(const int& count);
    glm::vec2 GetSplitAxis(const VoronoiCell& vc);


    float GetPointSize(VoronoiCell &cell) {return this->params.pointSize; }
    // points may be sized by any arbitrary function
    // of input image

    glm::vec3 GetPointColor(VoronoiCell &cell) { return glm::vec3(0,0,0); }

    //float AdaptivePointSize(const VoronoiCell &cell) {
    //    const float avgIntensitySqrt = std::sqrt(cell.m00 / cell.area);
    //    return this->params.pointSizeMin * (1.0f - avgIntensitySqrt) +
    //       this->params.pointSizeMax * avgIntensitySqrt;
    //}
};

#endif
