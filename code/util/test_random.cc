#include <CImg.h>
#include <common/types.h>
#undef Success
#include <Eigen/Dense>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>
#include "matlab.h"
#include "random.h"
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

DEFINE_int32(pdf_size, 100, "");
DEFINE_int32(samples, 1000, "");
DEFINE_double(width, 20.0f, "");

using Eigen::VectorXf;
using slib::util::Random;
using std::string;
using std::vector;

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  // Generate a Gaussian pdf.
  VectorXf pdf(FLAGS_pdf_size);
  for (int i = 0; i < pdf.size(); i++) {
    const float x = ((float) i) / ((float) pdf.size()) * FLAGS_width - FLAGS_width / 2.0f;
    pdf(i) = 1.0f / sqrt(2.0f * M_PI) * exp(-x*x / 2.0f);
  }
  {
    slib::util::MatlabMatrix matrix(pdf);
    matrix.SaveToFile("pdf.mat");
  }

  VectorXf samples = Random::SampleArbitraryDistribution(pdf, FLAGS_samples);
  {
    slib::util::MatlabMatrix matrix(samples);
    matrix.SaveToFile("samples.mat");
  }
  
  const float red[3] = {1.0f, 0.0f, 0.0f};
  const float green[3] = {0.0f, 1.0f, 0.0f};

  FloatImage pdf_plot(pdf.size(), 1);
  for (int i = 0; i < pdf.size(); i++) {
    pdf_plot(i) = pdf(i);
  }
  FloatImage samples_plot(pdf.size(), 1);
  samples_plot.fill(0.0f);
  const float num_bins = (float) samples_plot.size();
  const float bin_size = FLAGS_width / num_bins;
  for (int i = 0; i < samples.size(); i++) {
    const float sample = samples(i) + FLAGS_width / 2.0f;
    int bin = (int) floor(sample / bin_size);
    bin = bin < 0 ? 0 : bin;
    bin = bin >= samples_plot.size() ? samples_plot.size() - 1 : bin;

    samples_plot(bin) = samples_plot(bin) + 1.0f;
  }

  FloatImage plot(500, 400, 1, 3);
  plot.draw_graph(pdf_plot, red);
  plot.draw_graph(samples_plot, green).display();

  return 0;
}
