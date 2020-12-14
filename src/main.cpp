#include <iostream>
#include <string>

//#define cimg_use_jpeg
#include "CImg.h"

#include "stipples.h"
#include "glm/vec3.hpp"

using namespace cimg_library;

int main(int argc, char* argv[]) {
  // TODO: command line input and output
  CImg<unsigned char>* img1;

  try {
    img1 = new CImg<unsigned char>(argv[1]);
    std::cout << "Image successfully loaded" << std::endl;
  }
  catch(...) {
    std::cerr << "Failed to load image" << std::endl;
    return -1;
  }

  Params stippleParams(10, .001, .1,
                       .4, 2, 100,
                       2000, glm::vec3(255, 255, 255),
                       1);

  StippleImage stipple(*img1, stippleParams);
  stipple.Solve();
  stipple.DrawImage().save(argv[2]);


  delete img1;

  return 1;
}
