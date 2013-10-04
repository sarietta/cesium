#include "./matlabfunc/distrib/matlabfunc.h"
#include "matlab_function.h"

#include <common/types.h>
#undef Success
#include <Eigen/Dense>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include "matlab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>

using Eigen::MatrixXf;
using slib::util::MatlabFunction;
using slib::util::MatlabMatrix;
using std::string;

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  MatlabMatrix* A = new MatlabMatrix(MatrixXf::Random(3,3));
  MatlabMatrix* B = new MatlabMatrix();

  CREATE_MATLAB_FUNCTION(function, matlabfunc);
  function.Run(*A, B);
  LOG(INFO) << "A:\n" << *A;
  LOG(INFO) << "matlabfunc(A):\n" << *B;

  mclTerminateApplication();
}
