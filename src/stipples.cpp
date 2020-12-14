#include "stipples.h"

StippleImage::StippleImage(const CImg<unsigned char>& _img, const Params& _params) : params(_params) {
    std::random_device rd;
    generator = std::default_random_engine(rd());
    stipples = this->GetRandomStipples(this->params.count);

    img = CImg<unsigned char>(_img.width(), _img.height(), 1, 1, 0);

    voronoiSolver = new GPUVoronoi(img.width(), img.height());
    //voronoiSolver = new GPUVoronoi(img.width(), img.height());

    if(_img.spectrum() > 1) {
        cimg_forXY(_img,x,y) {

                // Separation of channels
                int R = (int)_img(x,y,0,0);
                int G = (int)_img(x,y,0,1);
                int B = (int)_img(x,y,0,2);
                //
                // Real weighted addition of channels for gray
                auto grayValueWeight = (unsigned char)(0.299*R + 0.587*G + 0.114*B);

                img(x,y,0,0) = grayValueWeight;
            }
    }
    else {
        img.assign(_img);
    }
}

glm::vec2 ClampPoint(glm::vec2 pt) {
    return glm::vec2(std::clamp(pt.x, 0.0f, 1.0f), std::clamp(pt.y, 0.0f, 1.0f));
}

void StippleImage::Iterate(float hysteresis) {
    std::vector<glm::vec2> pts = GetCenters(this->stipples);

    // TODO: factor out construction, image is always the same size
    cimg_library::CImg<unsigned char> map = voronoiSolver->GetImage(pts);

    std::vector<VoronoiCell> voronoi = GetVoronoiCells(map, img, pts);

    std::vector<Point> newPoints;
    // TODO: would heuristic be valuable?
    newPoints.reserve(stipples.size()*1.2);

    glm::vec2 center;
    glm::vec3 color;
    float size;

    this->changes = 0;
    for(int i : range(voronoi.size())) {
        size = this->GetPointSize(voronoi[i]);
        color = this->GetPointColor(voronoi[i]);

        if( voronoi[i].m00 < GetLowerSplitBound(size, hysteresis)
            || voronoi[i].area == 0 ) {

            // remove cell
            this->changes++;
        }
        else if( voronoi[i].m00 < GetUpperSplitBound(size, hysteresis) ) {

            // keep cell
            center = ClampPoint(voronoi[i].centroid);
            newPoints.emplace_back(center, size, color);
        }
        else {

            // split cell into 2
            glm::vec2 axis = GetSplitAxis(voronoi[i]);

            center = this->Jitter(voronoi[i].centroid + axis);
            newPoints.emplace_back(Point(ClampPoint(center), size, color));

            center = this->Jitter(voronoi[i].centroid - axis);
            newPoints.emplace_back(Point(ClampPoint(center), size, color));

            this->changes++;
        }
    }
    this->iterations++;
    std::cout << "iteration: " << this->iterations << " changes: " << this->changes << " stipples: " << newPoints.size() << std::endl;

    // TODO: memory copy problem?
    this->stipples = newPoints;
}


std::vector<Point> StippleImage::GetRandomStipples(const int& count) {
    std::uniform_real_distribution<float> distribution(0.0, 1.0);
    std::vector<Point> points;
    points.reserve(count);

    float x, y;
    glm::vec2 center;
    float size;
    glm::vec3 color;

    for( int i = 0; i < count; i++) {
        x = distribution(generator);
        y = distribution(generator);

        center = {x, y};
        size = 0;
        color = {0, 0, 0};

        points.emplace_back(center, size, color);
    }

    return points;
}


bool StippleImage::Solve() {
    while(!this->IsDone() && !this->IsError()) {
        float hysteresis = this->GetHysteresis();
        Iterate(hysteresis);
    }

    return !IsError();
}


bool StippleImage::IsDone() const {
    return (this->changes == 0);
}


bool StippleImage::IsError() {
    return (this->iterations >= this->params.maxIterations)
           || (this->stipples.size() > this->params.maxPoints);
}


glm::vec2 StippleImage::Jitter(glm::vec2 pt) {
    std::uniform_real_distribution<float> distribution(-1 * this->params.jitter,
                                                       this->params.jitter);

    float x, y;

    x = distribution(generator);
    y = distribution(generator);

    return pt + glm::vec2(x, y);
}


glm::vec2 StippleImage::GetSplitAxis(const VoronoiCell& vc) {
    std::vector<Point> pts;

    glm::vec2 center;

    float magnitude = std::max(1.0f, vc.area);
    magnitude /= PI;
    magnitude = std::sqrt(magnitude);
    magnitude /= 2.0f;

    // bootleg rotation matrix
    float x = std::cos(vc.angle) * magnitude / img.width();
    float y = std::sin(vc.angle) * magnitude / img.height();
    return glm::vec2(x, y);
}


CImg<unsigned char> StippleImage::DrawImage() {
    //int x, y;
    CImg<unsigned char> img(this->img.width(), this->img.height(), 1, 3);

    glm::vec3 bgdColor = this->params.bgdColor;
    cimg_forXY(img, x, y) img.fillC(x, y, 0, bgdColor.r, bgdColor.g, bgdColor.b);

    for(Point pt : this->stipples) {
        img.draw_circle((int) (pt.pos.x * img.width()),(int) (pt.pos.y * img.height()), pt.size, glm::value_ptr(pt.color));
    }

    return img;
}
