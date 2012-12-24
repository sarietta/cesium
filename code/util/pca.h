#ifndef __SLIB_UTIL_PCA_H__
#define __SLIB_UTIL_PCA_H__

#include <common/types.h>

namespace slib {
  namespace util {

    // Compute num_components principal components of the
    // samples. Samples should be row vectors so that the matrix
    // samples is a num_samples X dimensions matrix.
    FloatMatrix ComputePrincipalComponents(const FloatMatrix& samples, const int& num_components);

  }  // namespace util
}  // namespace slib

#endif
