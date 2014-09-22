#ifndef __SLIB_INTERPOLATION_FASTRBF_H__
#define __SLIB_INTERPOLATION_FASTRBF_H__

#include "rbf.h"

#include <common/types.h>
#include <gflags/gflags.h>
#undef Success
#include <Eigen/Dense>
#include <string>

namespace slib {
  namespace util {
    class MatlabMatrix;
  }
}

namespace slib {
  namespace interpolation {
    DECLARE_double(slib_interpolation_fastrbf_tolerance);
    DECLARE_int32(slib_interpolation_fastrbf_max_iterations);

    class FastMultiQuadraticRBF : public RadialBasisFunction {
    public:
      FastMultiQuadraticRBF();
      explicit FastMultiQuadraticRBF(const float& epsilon, const int& power);

      void ComputeWeights(const Eigen::VectorXf& values, const bool& normalized = false);
      float Interpolate(const Eigen::VectorXf& point);
#ifdef CUDA_ENABLED
      Eigen::VectorXf InterpolatePoints(const FloatMatrix& points);
#endif
      bool SaveToFile(const std::string& filename);
      bool LoadFromFile(const std::string& filename);

      slib::util::MatlabMatrix ConvertToMatlabMatrix() const;
      void LoadFromMatlabMatrix(const slib::util::MatlabMatrix& matrix);

      inline float GetAlpha() const {
	return _alpha;
      }
    private:
      float _epsilon;
      float _epsilon2;
      int _power;
      float _alpha;
    };
  }  // namespace interpolation
}  // namespace slib

#endif __SLIB_INTERPOLATION_FASTRBF_H__
