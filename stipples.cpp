#include "stipples.h"

StippleImage StippleImage::Iterate(float hysteresis) {
  std::vector<vec3> pts = GetCenters(this->stipples);
  std::vector<VoronoiCell> voronoi = GetVoronoiCells(img, pts);

  std::vector<Point> newPoints;
  newPoints.reserve(stipples.size());

  vec3 center, color;
  float size;

  this->changes = 0;
  for(int i : range(voronoi.size())) {
    if( voronoi[i].density < GetLowerSplitBound(this->stipples[i].size, hysteresis)
       || voronoi[i].area == 0 ) {

      // remove cell
      this->changes++;
    }
    else if( voronoi[i].density < GetUpperSplitBound(this->stipples[i].size, hysteresis) ) {

      // keep cell
      center = voronoi[i].centroid;
      size = this->GetPointSize(center);
      color = this->GetPointColor(center);
      newPoints.push_back(Point(center, size, color));
    }
    else {

      // split cell into 2
      std::vector<Point> splitPoints = this->SplitCell(voronoi[i]);

      newPoints.insert(newPoints.end(), splitPoints.begin(), splitPoints.end());
      this->changes++;
    }
  }
  this->iterations++;
  std::cout << "iteration: " << this->iterations << " changes: " << this->changes << " stipples: " << newPoints.size() << std::endl;

  std::string filename = std::to_string(this->iterations) + ".png";

  this->DrawImage().save(filename.c_str());

  // TODO: memory copy problem?
  this->stipples = newPoints;
  return *this;
}


std::vector<Point> StippleImage::GetRandomStipples(const int& count) {
  std::default_random_engine generator;
  std::uniform_real_distribution<float> distribution(0.0, 1.0);
  std::vector<Point> points;
  points.reserve(count);

  float x, y;
  vec3 center;
  float size;
  vec3 color;

  for( int i = 0; i < count; i++) {
    x = distribution(generator);
    y = distribution(generator);

    center = vec3(x, y, 0);
    size = this->GetPointSize(center);
    color = this->GetPointColor(center);

    points.push_back(Point(center, size, color));
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


bool StippleImage::IsDone() {
  return (this->changes == 0);
}


bool StippleImage::IsError() {
  return (this->iterations >= this->params.maxIterations)
      || (this->stipples.size() > this->params.maxPoints);
}


vec3 StippleImage::Jitter(vec3 pt) {
  std::default_random_engine generator;
  std::uniform_real_distribution<float> distribution(-1 * this->params.jitter,
                                                          this->params.jitter);

  float x, y;

  x = distribution(generator);
  y = distribution(generator);

  return pt + vec3(x, y, 0);
}


std::vector<Point> StippleImage::SplitCell(const VoronoiCell& vc) {
  std::vector<Point> pts;

  vec3 center, color;
  float size;

  center = this->Jitter(vc.centroid + vc.axis);

  size   = this->GetPointSize(center);
  color  = this->GetPointColor(center);
  pts.push_back(Point(center, size, color));

  center = this->Jitter(vc.centroid - vc.axis);
  size   = this->GetPointSize(center);
  color  = this->GetPointColor(center);
  pts.push_back(Point(center, size, color));

  return pts;
}


CImg<short> StippleImage::DrawImage() {
  int x, y;
  CImg<short> img(this->img.width(), this->img.height(), 1, 3);

  vec3 bgdColor = this->params.bgdColor;
  cimg_forXY(img, x, y) img.fillC(x, y, 0, bgdColor.r(), bgdColor.g(), bgdColor.b());

  for(Point pt : this->stipples) {
    img.draw_circle((int) (pt.pos[0] * img.width()),(int) (pt.pos[1] * img.height()), pt.size, pt.color.vals());
  }

  return img;
}
