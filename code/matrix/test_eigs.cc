#include <common/types.h>
#undef Success
#include <Eigen/Dense>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>
#include "eigs.h"
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

using Eigen::SelfAdjointEigenSolver;
using Eigen::VectorXf;
using slib::matrix::EigenSolver;
using std::string;
using std::vector;

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  FLAGS_logtostderr = true;

  FloatMatrix A = FloatMatrix::Random(4, 4);
  A = A * A.transpose();
  SelfAdjointEigenSolver<FloatMatrix> eigensolver(A);

  bool reverse = true;
  if (eigensolver.info() != Eigen::Success) {
    LOG(ERROR) << "Eigenvalues did not converge... results may be weird.";
    reverse = false;
  }
  
  {
    VectorXf eigenvalues;
    FloatMatrix eigenvectors;
    if (reverse) {
      eigenvalues = eigensolver.eigenvalues().reverse();
      eigenvectors = eigensolver.eigenvectors().rowwise().reverse();
    } else {
      eigenvalues = eigensolver.eigenvalues();
      eigenvectors = eigensolver.eigenvectors();
    }
    
    LOG(INFO) << "\nA:\n" << A;
    LOG(INFO) << "\n\nEigenvalues:\n" << eigenvalues;
    LOG(INFO) << "\n\nEigenvectors:\n" << eigenvectors;
  }

  LOG(INFO) << "\n\n==============================================\n\n";
  
  {
    const int neigs = 2;
    FloatMatrix eigenvalues(neigs, 1);
    FloatMatrix eigenvectors(neigs, A.rows());

    EigenSolver::eigs(A, neigs, &eigenvalues, &eigenvectors);
    eigenvectors.transposeInPlace();

    LOG(INFO) << "\nA:\n" << A;
    LOG(INFO) << "\n\nEigenvalues:\n" << eigenvalues;
    LOG(INFO) << "\n\nEigenvectors:\n" << eigenvectors;
  }

  return 0;
}
