#ifndef __SLIB_INTERPOLATION_RBF_H__
#define __SLIB_INTERPOLATION_RBF_H__

#include "../common/types.h"
#include <Eigen/Dense>
#include <string>

typedef float(*RadialBasisFunction_Function)(const float& r);

namespace slib {
  namespace interpolation {

    class RadialBasisFunction {
    public:
      RadialBasisFunction(float(*rbf)(const float& r));
      virtual ~RadialBasisFunction() {}

      void SetPoints(const Eigen::MatrixXf& points);
      void ComputeWeights(const Eigen::VectorXf& values, const bool& normalized = false);
      float Interpolate(const Eigen::VectorXf& point);

      bool SaveToFile(const std::string& filename);
      bool LoadFromFile(const std::string& filename);

      inline const Eigen::VectorXf GetPoint(const int32& idx) const {
	return _points.row(idx);
      }

      inline float GetWeight(const int32& idx) const {
	return _w(idx);
      }
      
      inline RadialBasisFunction_Function GetFunction() const {
	return _rbf;
      }

      inline int32 GetNumPoints() const {
	return _N;
      }

      inline int32 GetNumDimensions() const {
	return _dimensions;
      }

    private:
      int32 _dimensions;
      int32 _N;
      float(*_rbf)(const float& r);
      bool _normalized;

      Eigen::VectorXf _w;
      Eigen::MatrixXf _points;
    };

  }  // namespace interpolation
}  // namespace slib

#endif
