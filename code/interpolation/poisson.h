#ifndef __SLIB_INTERPOLATION_POISSON_H__
#define __SLIB_INTERPOLATION_POISSON_H__

#define EIGEN_YES_I_KNOW_SPARSE_MODULE_IS_NOT_STABLE_YET

#include "../common/types.h"
#include <Eigen/Sparse>
#include <string>

typedef float(*CouplingWeight)(const int32& i, const int32& j);

namespace slib {
  namespace interpolation {

    class PoissonInterpolation {
    public:
      PoissonInterpolation(float(*coupling_weight_functioni)(const int32&, const int32&));
      virtual ~PoissonInterpolation() {}

      void SetPoints(const Eigen::MatrixXf& points);
      void SetRegularizationWeight(const float& lambda);

      bool Interpolate(const Eigen::MatrixXf* interpolated) const;

      inline int32 GetNumPoints() const {
	return _N;
      }

      inline int32 GetNumDimensions() const {
	return _dimensions;
      }

      inline float GetRegularizationWeight() const {
	return _lambda;
      }

    private:
      int32 _dimensions;
      int32 _N;

      Eigen::MatrixXf _points;
      float(*_coupling_weight_function)(const int32&, const int32&);
      float _lambda;
    };

  }  // namespace interpolation
}  // namespace slib

#endif
