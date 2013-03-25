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
DEFINE_double(height, 20.0f, "");

DEFINE_int32(point_radius, 10, "");

DEFINE_bool(test1d, false, "");
DEFINE_bool(test2d, true, "");

using Eigen::VectorXf;
using slib::util::Random;
using std::string;
using std::vector;

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  if (FLAGS_test1d) {
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
    for (int i = 0; i < samples.size(); i++) {
      const int sample = samples(i);
      samples_plot(sample) = samples_plot(sample) + 1.0f;
    }
    
    FloatImage plot(500, 400, 1, 3);
    plot.draw_graph(pdf_plot, red);
    plot.draw_graph(samples_plot, green).display();
  }

  if (FLAGS_test2d) {
    // Generate a Gaussian pdf.
    FloatMatrix pdf(FLAGS_pdf_size, FLAGS_pdf_size);
    for (int i = 0; i < pdf.rows(); i++) {
      for (int j = 0; j < pdf.cols(); j++) {
	const float x = ((float) i) / ((float) pdf.cols()) * FLAGS_width - FLAGS_width / 2.0f;
	const float y = ((float) j) / ((float) pdf.rows()) * FLAGS_height - FLAGS_height / 2.0f;
	pdf(i, j) = 1.0f / (2.0f * M_PI) * exp(-(x*x + y*y) / 2.0f);
      }
    }
    {
      slib::util::MatlabMatrix matrix(pdf);
      matrix.SaveToFile("pdf2d.mat");
    }
    
    FloatMatrix samples = Random::SampleArbitraryDistribution(pdf, FLAGS_samples);
    {
      slib::util::MatlabMatrix matrix(samples);
      matrix.SaveToFile("samples2d.mat");
    }
        
    const VectorXf marginal = pdf.rowwise().sum() / ((float) pdf.cols());

    FloatImage pdf_plot(pdf.cols(), pdf.rows(), 1, 3);
    FloatImage marginal_plot(1, pdf.rows(), 1, 3);
    for (int i = 0; i < pdf.rows(); i++) {
      marginal_plot(0, i, 0, 0) = marginal(i);
      marginal_plot(0, i, 0, 1) = 0.0f;
      marginal_plot(0, i, 0, 2) = 0.0f;
      for (int j = 0; j < pdf.cols(); j++) {
	pdf_plot(j, i, 0, 0) = pdf(i, j);
	pdf_plot(j, i, 0, 1) = 0.0f;
	pdf_plot(j, i, 0, 2) = 0.0f;
      }
    }
    FloatImage samples_plot(pdf.cols(), pdf.rows(), 1, 3);
    samples_plot.fill(0.0f);

    const float green[3] = {0.0f, 1.0f, 0.0f};

    for (int i = 0; i < samples.rows(); i++) {
      int x = samples(i, 0);
      int y = samples(i, 1);
      
      samples_plot.draw_circle(x, y, FLAGS_point_radius, green);
    }
    
    //(marginal, pdf_plot).display();
    pdf_plot.display();
    samples_plot.display();
  }
  
  return 0;
}
