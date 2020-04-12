#include <iostream>
#include <string>
#include "CImg.h"

#include "vec3.h"
#include "stipples.h"

using namespace cimg_library;

int main() {
  CImg<unsigned char>* img1;
  std::string source = "clocktower.jpg";

  try {
    img1 = new CImg<unsigned char>(source.c_str());
    std::cout << "Image successfully loaded" << std::endl;
  }
  catch(...) {
    std::cerr << "Failed to load image" << std::endl;
    return -1;
  }

  Params stippleParams(100, .0005, .02, .6, 1.5, 100, 150000, vec3(255, 255, 255), 1);

  StippleImage stipple(*img1, stippleParams);

  CImg<unsigned char> img2 = stipple.DrawImage();
  img2.save("0.png");

  stipple.Solve();
  img2 = stipple.DrawImage();
  img2.save("final.png");

  delete img1;

  return 1;
}
