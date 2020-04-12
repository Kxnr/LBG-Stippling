#include "gpuVoronoi.h"
#include "CImg.h"
#include "glm/glm.hpp"
#include "glm/vec2.hpp"
#include "utils.h"

int main() {
  std::vector<glm::vec2> positions = {{0.5f, 0.3f}, {0.66f, 0.66f}, {0.33f, 0.66f}};

  cimg_library::CImg<unsigned char> testImg(1000, 500, 1, 3);
  GPUVoronoi test = GPUVoronoi(testImg.width(), testImg.height());

  unsigned char color[3] = {127, 127, 127};

  cimg_library::CImg<unsigned char> map = test.GetImage(positions);

  cimg_library::CImg<unsigned char> diff = cimg_library::CImg<unsigned char>(testImg.width(), testImg.height(), 1, 3, 0);
  cimg_library::CImg<unsigned char> gpu = cimg_library::CImg<unsigned char>(testImg.width(), testImg.height(), 1, 3, 0);
  cimg_library::CImg<unsigned char> cpu = cimg_library::CImg<unsigned char>(testImg.width(), testImg.height(), 1, 3, 0);

  float dist, minDist, density;
  float fx, fy; // normalized coordinates
  int x, y, ind;
  int index;
  float aspect = testImg.width() / (float) testImg.height();

  cimg_forXY(testImg, x, y) {

    fx = x / (float) testImg.width();
    fy = y / (float) testImg.height();

    minDist = std::numeric_limits<float>::infinity();
    dist = 0;

    for(int i = 0 ; i < positions.size(); i++) {
      dist = std::sqrt((fx-positions[i][0])*(fx-positions[i][0])*aspect*aspect + (fy-positions[i][1])*(fy-positions[i][1]) );
      if(dist < minDist) {
        minDist = dist;
        ind = i;
      }
    }

    index = DecodeColor(map(x, y, 0), map(x, y, 1), map(x, y, 2));

    if(index == 0) {

      gpu = gpu.fillC(x, y, 0, 255, 0, 0);
    }
    else if(index == 1) {
      gpu = gpu.fillC(x, y, 0, 0, 255, 0);
    }
    else if(index ==2) {
      gpu = gpu.fillC(x, y, 0, 0, 0, 255);
    }
    else {
      gpu = gpu.fillC(x, y, 0, 255, 255, 255);
    }

    if(ind == 0) {
      cpu = cpu.fillC(x, y, 0, 255, 0, 0);
    }
    else if(ind == 1) {
      cpu = cpu.fillC(x, y, 0, 0, 255, 0);
    }
    else if(ind ==2) {
      cpu = cpu.fillC(x, y, 0, 0, 0, 255);
    }
    else {
      cpu = cpu.fillC(x, y, 0, 255, 255, 255);
    }

    std::cout << index << std::endl;
    if(index < 0 || index > 2) {
      std::cout << index << std::endl;
      std::cout << "problem" << std::endl;
    }

    if(ind != index) {
      diff = diff.fillC(x, y, 0, 255, 0, 0);
    }
    else {
      diff = diff.fillC(x, y, 0, 0, 0, 0);
    }
  }

  for(glm::vec2 pt : positions) {
    diff.draw_circle((int) (pt.x * diff.width()),(int) (pt.y * diff.height()), 2, color);
    cpu.draw_circle((int) (pt.x * diff.width()),(int) (pt.y * diff.height()), 2, color);
    gpu.draw_circle((int) (pt.x * diff.width()),(int) (pt.y * diff.height()), 2, color);
    map.draw_circle((int) (pt.x * diff.width()),(int) (pt.y * diff.height()), 2, color);
  }

  diff.save("diff.png");
  cpu.save("cpu.png");
  gpu.save("gpu.png");

  test.GetImage().save("test_0.png");
  test.DrawCones(positions);
  test.DrawCones(positions);
  test.DrawCones(positions);
  test.DrawCones(positions);
  test.DrawCones(positions);
  test.DrawCones(positions);
  test.DrawCones(positions);
  test.DrawCones(positions);
  test.DrawCones(positions);
  test.DrawCones(positions);
  test.DrawCones(positions);
  test.DrawCones(positions);
  test.DrawCones(positions);
  test.GetImage().save("test_n.png");

  cimg_forXY(map, x, y) {
    // std::cout << DecodeColor(map(x, y, 0), map(x, y, 1), map(x, y, 2)) << std::endl;
  }

}
