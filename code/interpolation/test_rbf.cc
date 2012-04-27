#include <CImg.h>
#include "../common/types.h"
#undef Success
#include <Eigen/Dense>
#include <glog/logging.h>
#include "rbf.h"
#include <stdio.h>
#include <stdlib.h>
#include "../util/colormap.h"

using namespace cimg_library;
using namespace Eigen;

using slib::interpolation::RadialBasisFunction;

float MultiQuadric(const float& r) {
  return sqrt(r*r + 0.01);
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);

  srand(128);

  const float max = 2.0;
  const float min = -2.0;
  const float range = max - min;

  const int N = 100;
  const int D = 2;
  MatrixXf points(N, D);
  VectorXf values(N);

  float max_value = 0.0f;
  float min_value = 0.0f;
  for (int i = 0; i < N; i++) {
    const float x = static_cast<float>(rand()) / RAND_MAX * range + min;
    const float y = static_cast<float>(rand()) / RAND_MAX * range + min;
    points(i,0) = x;
    points(i,1) = y;

    const float value = x * exp(-(x*x) - y*y);
    values(i) = value;

    if (value > max_value) {
      max_value = value;
    }
    if (value < min_value) {
      min_value = value;
    }
  }

  RadialBasisFunction rbf(&MultiQuadric);
  rbf.SetPoints(points);
  rbf.ComputeWeights(values);

  const float sampling_density = 100.0f;
  const int width = static_cast<int>(sampling_density * range * 2);
  const int height = static_cast<int>(sampling_density * range * 2);
  const float centerx = static_cast<float>(width) / 2.0f;
  const float centery = static_cast<float>(height) / 2.0f;
  float color[3];

  CImgDisplay display;
  FloatImage interpolated_and_original_points(width, height, 1, 3, 255.0f);
  FloatImage interpolated_points(width, height, 1, 3, 255.0f);
  cimg_forXYC(interpolated_points, x, y, c) {
    Vector2f point((x - centerx) / sampling_density, (y - centery) / sampling_density);
    const float value = rbf.Interpolate(point);
    const float v = (value - min_value) / (max_value - min_value);

    slib::util::ColorMap::jet(v, color);
    interpolated_points(x, y, 0, c) = 255.0f * color[c];
    interpolated_and_original_points(x, y, 0, c) = 255.0f * color[c];
  }

  FloatImage original_points(width, height, 1, 3, 255.0f);
  for (int i = 0; i < N; i++) {
    const float x = points(i,0) * sampling_density;
    const float y = points(i,1) * sampling_density;
    const float v = (values(i) - min_value) / (max_value - min_value);
    
    slib::util::ColorMap::jet(v, color);
    for (int c = 0; c < 3; c++) { color[c] *= 255.0f; }
    original_points.draw_circle(x + centerx, y + centery, width / 80, color);
    interpolated_and_original_points.draw_circle(x + centerx, y + centery, width / 80, color);
  }
  
  (original_points, interpolated_points, interpolated_and_original_points).display();

  return 0;
}
