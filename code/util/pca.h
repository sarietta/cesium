#ifndef __SLIB_UTIL_PCA_H__
#define __SLIB_UTIL_PCA_H__

#include <common/types.h>
#include <Eigen/Dense>

namespace slib {
  namespace util {

    /**
       Computes principal components of a set of samples.

       @parameter samples - a matrix of samples where each row represents one sample. Thus the dimensions of this matrix should be num_samples x dimensions.
       @parameter num_components - the number of principle components to compute.
       @parameter eigenvalues - returns the corresponding eigenvalues for the principle components as a vector. This should be pre-allocated to have num_components entries.
    **/
    FloatMatrix ComputePrincipalComponents(const FloatMatrix& samples, const int& num_components,
					   FloatMatrix* eigenvalues);

  }  // namespace util
}  // namespace slib

#endif
