#include <CImg.h>
#include "../common/types.h"
#undef Success
#include <Eigen/Dense>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include "rbf.h"
#include <stdio.h>
#include <stdlib.h>
#include "../util/colormap.h"

#define E_2_1 0.13533528323

using namespace cimg_library;
using namespace Eigen;

DEFINE_int32(num_points, 100, "");
DEFINE_double(radius, 0.01, "");

DEFINE_double(density, 100.0, "");
DEFINE_double(xmin, -2.0, "");
DEFINE_double(xmax, 2.0, "");
DEFINE_double(ymin, -2.0, "");
DEFINE_double(ymax, 2.0, "");

DEFINE_bool(use_alt, true, "");
DEFINE_bool(use_gaussian, true, "");
DEFINE_bool(use_thinplate, false, "");

using slib::interpolation::RadialBasisFunction;

float MultiQuadric(const float& r) {
  return sqrt(r*r + FLAGS_radius);
}

inline float GaussianRBF(const float& r) {
  const double r1 = 2.0 * FLAGS_radius * FLAGS_radius * E_2_1;
  const float value = exp(-r * r / r1);
  return value;
}

inline float ThinPlateRBF(const float& r) {
  if (r <= 0) {
    return 0;
  }
  const float value = r * r * log(r / FLAGS_radius);
  return value;
}

inline void MultiQuadricAlt(int n, double* r, double r0, double* v) {
  for (int i = 0; i < n; i++) {
    v[i] = sqrt(r[i] * r[i] + r0);
  }
}

inline void GaussianRBFAlt(int n, double* r, double r0, double* v) {
  const double r1 = 2.0 * r0 * r0 * E_2_1;
  for (int i = 0; i < n; i++) {
    v[i] = exp(-r[i] * r[i] / r1);
  }
}

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  srand(128);

  const float xmax = FLAGS_xmax;
  const float xmin = FLAGS_xmin;
  const float xrange = xmax - xmin;

  const float ymax = FLAGS_ymax;
  const float ymin = FLAGS_ymin;
  const float yrange = ymax - ymin;

  LOG(INFO) << "xrange: " << xrange;
  LOG(INFO) << "yrange: " << yrange;

  const int N = FLAGS_num_points;
  const int D = 2;
  MatrixXf points(N, D);
  VectorXf values(N);

  float max_value = 0.0f;
  float min_value = 0.0f;
  for (int i = 0; i < N; i++) {
    const float x = static_cast<float>(rand()) / RAND_MAX * xrange + xmin;
    const float y = static_cast<float>(rand()) / RAND_MAX * yrange + ymin;
    points(i,0) = x;
    points(i,1) = y;

#if 1
    //const float value = (i % 2 == 0) ? 0.0 : 1.0f;
    const float value = static_cast<float>(rand()) / RAND_MAX;
#else
    const float value = x * exp(-(x*x) - y*y);
#endif
    values(i) = value;

    if (value > max_value) {
      max_value = value;
    }
    if (value < min_value) {
      min_value = value;
    }
  }

  RadialBasisFunction* rbf;
  if (FLAGS_use_alt) {
    FLAGS_rbf_interpolation_radius = FLAGS_radius;
    if (FLAGS_use_gaussian) {
      rbf = new RadialBasisFunction(&GaussianRBFAlt);
    } else {
      rbf = new RadialBasisFunction(&MultiQuadricAlt);
    }
  } else {
    if (FLAGS_use_gaussian) {
      rbf = new RadialBasisFunction(&GaussianRBF);
    } else if (FLAGS_use_thinplate) {
      rbf = new RadialBasisFunction(&ThinPlateRBF);
    } else {
      rbf = new RadialBasisFunction(&MultiQuadric);
    }
  }

  rbf->EnableConditionTest();
  rbf->SetPoints(points);
  rbf->ComputeWeights(values);

  const int width = static_cast<int>(xrange);
  const int height = static_cast<int>(yrange);
  const float black[3] = {0, 0, 0};
  float color[3];

  CImgDisplay display;
  FloatImage interpolated_and_original_points(width, height, 1, 3, 255.0f);
  FloatImage interpolated_points(width, height, 1, 3, 255.0f);
  cimg_forXYC(interpolated_points, x, y, c) {
    Vector2f point(x + xmin, y + ymin);
    const float value = rbf->Interpolate(point);
    const float v = (value - min_value) / (max_value - min_value);

    slib::util::ColorMap::jet(v, color);
    interpolated_points(x, y, 0, c) = 255.0f * color[c];
    interpolated_and_original_points(x, y, 0, c) = 255.0f * color[c];
  }

  FloatImage original_points(width, height, 1, 3, 255.0f);
  for (int i = 0; i < N; i++) {
    const float x = points(i,0) + xmin;
    const float y = points(i,1) + ymin;
    const float v = (values(i) - min_value) / (max_value - min_value);
    
    slib::util::ColorMap::jet(v, color);
    for (int c = 0; c < 3; c++) { color[c] *= 255.0f; }
    original_points.draw_circle(x + xmin, y + ymin, width / 80, color);
    interpolated_and_original_points.draw_circle(x + xmin, y + ymin, width / 80 + 2, black);
    interpolated_and_original_points.draw_circle(x + xmin, y + ymin, width / 80, color);
  }
  
  (original_points, interpolated_points, interpolated_and_original_points).display();

  return 0;
}
