#ifndef __SLIB_INTERPOLATION_RBF_H__
#define __SLIB_INTERPOLATION_RBF_H__

#include <common/types.h>
#include <gflags/gflags.h>
#include <Eigen/Dense>
#include <string>

DECLARE_double(rbf_interpolation_radius);

typedef float(*RadialBasisFunction_Function)(const float& r);
typedef void(*RadialBasisFunction_AltFunction)(int n, double* r, double r0, double* v);

namespace slib {
  namespace interpolation {

    class RadialBasisFunction {
    public:
      RadialBasisFunction(RadialBasisFunction_Function rbf);
      RadialBasisFunction(RadialBasisFunction_AltFunction rbf);
      virtual ~RadialBasisFunction() {}

      void EnableConditionTest();

      void SetPoints(const Eigen::MatrixXf& points);
      void ComputeWeights(const Eigen::VectorXf& values, const bool& normalized = false);
      float Interpolate(const Eigen::VectorXf& point);

      void ComputeWeightsAlt(const Eigen::VectorXf& values, const bool& normalized = false);
      float InterpolateAlt(const Eigen::VectorXf& point);

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
      RadialBasisFunction_Function _rbf;
      RadialBasisFunction_AltFunction _alt_rbf;
      bool _normalized;

      bool _condition_test;

      Eigen::VectorXf _w;
      Eigen::MatrixXf _points;
    };

  }  // namespace interpolation
}  // namespace slib

#endif
