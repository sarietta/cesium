#ifndef __SLIB_INTERPOLATION_FAST_RBF_CU_H__
#define __SLIB_INTERPOLATION_FAST_RBF_CU_H__

#include "cuda-helpers.h"

#include <Eigen/Dense>

namespace slib {
  namespace interpolation {
    Eigen::VectorXf CUDAInterpolatePoints(const Eigen::MatrixXf& _points, const Eigen::VectorXf& _w,
					  const RadialBasisFunction::Function& _rbf,
					  const Eigen::MatrixXf& _samples);
  }  // namepsace interpolation
}  // namespace slib

#endif  // __SLIB_INTERPOLATION_FAST_RBF_CU_H__
