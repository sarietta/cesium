#include <CImg.h>
#include <common/types.h>
#undef Success
#include <Eigen/Sparse>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <map>
#include "matlab.h"
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

DEFINE_string(matrix, "sparse.mat", "File location of a sparse matrix");

using slib::util::MatlabMatrix;
using std::map;
using std::string;

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  MatlabMatrix matrix = MatlabMatrix::LoadFromFile(FLAGS_matrix);
  SparseFloatMatrix sfMatrix = matrix.GetCopiedSparseContents();

  LOG(INFO) << "Sparse matrix: \n" << sfMatrix;

  return 0;
}
