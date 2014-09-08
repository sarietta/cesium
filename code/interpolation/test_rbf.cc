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

DEFINE_bool(display_enabled, true, "");
DEFINE_string(output_file, "", "If non-empty, interpolated output saved here.");

using slib::interpolation::RadialBasisFunction;

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
#if 0
    if (FLAGS_use_gaussian) {
      rbf = new RadialBasisFunction(slib::interpolation::GaussianRBF(FLAGS_radius));
    } else {
      rbf = new RadialBasisFunction(slib::interpolation::MultiQuadricAlt);
    }
#endif
  } else {
    if (FLAGS_use_gaussian) {
      rbf = new RadialBasisFunction(slib::interpolation::GaussianRBF(FLAGS_radius));
    } else if (FLAGS_use_thinplate) {
      rbf = new RadialBasisFunction(slib::interpolation::ThinPlateRBF(FLAGS_radius));
    } else {
      rbf = new RadialBasisFunction(slib::interpolation::MultiQuadric(FLAGS_radius));
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
  if (FLAGS_display_enabled) {
    (original_points, interpolated_points, interpolated_and_original_points).display();
  }

  if (FLAGS_output_file != "") {
    interpolated_points.save(FLAGS_output_file.c_str());
  }

  return 0;
}
