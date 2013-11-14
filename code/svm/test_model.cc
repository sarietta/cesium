#include <CImg.h>
#include <common/types.h>
#undef Success
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <map>
#include "model.h"
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "svm.h"
#include <util/matlab.h>
#include <util/stl.h>

DEFINE_string(matlab_model, "model.mat", "File location of a model trained in MATLAB");

using Eigen::VectorXf;
using slib::svm::Model;
using slib::util::MatlabMatrix;
using std::map;
using std::string;

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  FloatMatrix samples(4, 2);
  samples << 
    2, 0, 
    3, 1, 
    4, 5, 
    5, 6;
  VectorXf labels(4);
  labels << -1, -1, +1, +1;

  Model model;
  slib::svm::Train(labels, samples, "-s 0 -t 0", &model);
  LOG(INFO) << "Trained model weights: " 
	    << slib::util::PrintVector(slib::util::PointerToVector(model.weights.get(), model.num_weights));

  Model matlab_model(MatlabMatrix::LoadFromFile(FLAGS_matlab_model));
  LOG(INFO) << "MATLAB model weights: " 
	    << slib::util::PrintVector(slib::util::PointerToVector(matlab_model.weights.get(), 
								   matlab_model.num_weights));

  return 0;
}
