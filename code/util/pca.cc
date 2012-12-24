#include "pca.h"

#include <Eigen/Dense>
#include <glog/logging.h>
#include <util/assert.h>

using Eigen::SelfAdjointEigenSolver;
using Eigen::VectorXf;

namespace slib {
  namespace util {

    FloatMatrix ComputePrincipalComponents(const FloatMatrix& samples, const int& num_components,
					   FloatMatrix* eigenvalues) {
      const int num_samples = samples.rows();
      const int dimensions = samples.cols();

      ASSERT_LTE(num_components, dimensions);
      
      FloatMatrix covariance;
      {
	// Compute the mean in each dimension.
	FloatMatrix mean = samples.colwise().mean();      
	// Subtract mean from the samples.
	FloatMatrix whitened = samples - mean.replicate(num_samples, 1);
	// Compute covariance.
	covariance = whitened.transpose() * whitened;
	covariance /= (float) (num_samples - 1);
      }

      // Compute the eigenvectors of the covariance matrix.
      SelfAdjointEigenSolver<FloatMatrix> eigensolver(covariance);
      if (eigensolver.info() != Eigen::Success) {
	LOG(ERROR) << "Could not create an eigensolver";
	return FloatMatrix(1,1);
      }

      if (eigenvalues != NULL) {
	VectorXf eigenvalues_ = eigensolver.eigenvalues();
	for (int i = 0; i < num_components; i++) {
	  (*eigenvalues)(i) = eigenvalues_(i);
	} 
      }

      return eigensolver.eigenvectors().leftCols(num_components);
    }

  }  // namespace util
}  // namespace slib
