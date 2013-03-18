#ifndef __SLIB_MATRIX_EIGS_H__
#define __SLIB_MATRIX_EIGS_H__

#include <common/scoped_ptr.h>
#include <common/types.h>
#undef Success
#include <Eigen/Dense>

namespace slib {
  namespace matrix {

    class EigenSolver {
    public:
      static int eigs(const FloatMatrix& A, const int& neigs, 
		      FloatMatrix* eigvals, FloatMatrix* eigvecs);

      static int eigs(const float* A, const int& N,
		      const int& neigs, float* eigvals, float* eigvecs);

      static int eigs(const DoubleMatrix& A, const int& neigs, 
		      DoubleMatrix* eigvals, DoubleMatrix* eigvecs);

      static int eigs(const double* A, const int& N,
		      const int& neigs, double* eigvals, double* eigvecs);
    };
  }  // namespace matrix
}  // namespace slib

#endif
