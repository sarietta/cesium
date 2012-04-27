#include "poisson.h"

#include "../util/assert.h"
#include <Eigen/Sparse>
//#include <unsupported/Eigen/src/SparseExtra/SuperLUSupport.h>
#include <glog/logging.h>
#include <string>

using namespace Eigen;
using std::string;

namespace slib {
  namespace interpolation {

    PoissonInterpolation::PoissonInterpolation(float(*coupling_weight_function)(const int32&, const int32&)) {
      _coupling_weight_function = coupling_weight_function;
    }

    void PoissonInterpolation::SetPoints(const MatrixXf& points) {
      _points = points;
      _N = _points.rows();
      _dimensions = _points.cols();      
    }

    void PoissonInterpolation::SetRegularizationWeight(const float& lambda) {
      _lambda = lambda;
    }
    
    bool PoissonInterpolation::Interpolate(const MatrixXf* interpolated) const {
      DynamicSparseMatrix<float> Aproxy(_N, _N);
      //Aproxy.reserve(num_non_zero);
      for (int32 i = 0; i < _N-1; i++) {
	Aproxy.insert(i,i+0) = +1;
	Aproxy.insert(i,i+1) = -1;
	//Aproxy.insertBack(i,i+0) = +1;
	//Aproxy.insertBack(i,i+1) = -1;
      }
      SparseMatrix<float> A(Aproxy);

      VectorXf b = VectorXf::Zero(_N);
#if 0
      SparseLU<SparseMatrix<float> > lu;
      lu.compute(A);

      const uint32 height = interpolated->rows();
      const uint32 width = interpolated->cols();

      VectorXf interpolatedV(_N);
      if (lu.solve(b, interpolatedV)) {
	for (uint32 x = 0; x < width; x++) {
	  for (uint32 y = 0; y < height; y++) {
	    (*interpolated)(x, y) = interpolatedV(y + x*height);
	  }
	}

	return true;
      } else {
#endif
	LOG(WARNING) << "Could not compute the LU decomposition of the weight matrix";
	return false;
#if 0
      }
#endif
    }

  }  // namespace interpolation
}  // namespace slib
