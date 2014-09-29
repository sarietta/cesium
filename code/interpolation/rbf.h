#ifndef __SLIB_INTERPOLATION_RBF_H__
#define __SLIB_INTERPOLATION_RBF_H__

#include <common/types.h>
#include <gflags/gflags.h>
#include <Eigen/Dense>
#include <string>

DECLARE_double(rbf_interpolation_radius);

typedef float(*RadialBasisFunction_FunctionPtr)(const float& epsilon, const float& r);
typedef void(*RadialBasisFunction_AltFunctionPtr)(int n, double* r, double r0, double* v);

namespace slib {
  namespace util {
    class MatlabMatrix;
  }
}

namespace slib {
  namespace interpolation {

    class RadialBasisFunction {
    public:
      // Functor for RBF functions.
      struct Function {
	float epsilon;
	RadialBasisFunction_FunctionPtr function;
	bool valid;
	Function() : valid(false) {}
	Function(const float& epsilon_, RadialBasisFunction_FunctionPtr function_ptr) 
	  : epsilon(epsilon_), function(function_ptr) {}

	inline float evaluate(const float& r) {
	  return (*function)(epsilon, r);
	}
      };

      struct AltFunction {
	float epsilon;
	RadialBasisFunction_AltFunctionPtr function;
	bool valid;
	AltFunction() : valid(false) {}
	AltFunction(const float& epsilon_, RadialBasisFunction_AltFunctionPtr function_ptr) 
	  : epsilon(epsilon_), function(function_ptr) {}	
      };

      RadialBasisFunction(const Function& rbf);
      RadialBasisFunction(const AltFunction& rbf);
      virtual ~RadialBasisFunction() {}

      void EnableConditionTest();

      virtual void SetPoints(const Eigen::MatrixXf& points);
      virtual void ComputeWeights(const Eigen::VectorXf& values, const bool& normalized = false);
      virtual float Interpolate(const Eigen::VectorXf& point);
      virtual Eigen::VectorXf InterpolatePoints(const FloatMatrix& points);

      void ComputeWeightsAlt(const Eigen::VectorXf& values, const bool& normalized = false);
      float InterpolateAlt(const Eigen::VectorXf& point);

      virtual bool SaveToFile(const std::string& filename);
      virtual bool LoadFromFile(const std::string& filename);

      virtual slib::util::MatlabMatrix ConvertToMatlabMatrix() const;

      inline const Eigen::VectorXf GetPoint(const int32& idx) const {
	return _points.row(idx);
      }

      inline float GetWeight(const int32& idx) const {
	return _w(idx);
      }
      
      inline RadialBasisFunction_FunctionPtr GetFunction() const {
	return _rbf.function;
      }

      inline int32 GetNumPoints() const {
	return _N;
      }

      inline int32 GetNumDimensions() const {
	return _dimensions;
      }

    protected:
      RadialBasisFunction();

      int32 _dimensions;
      int32 _N;
      Function _rbf;
      AltFunction _alt_rbf;
      bool _normalized;

      bool _condition_test;

      Eigen::VectorXf _w;
      Eigen::MatrixXf _points;
    };

    // Some RBFs pre-defined.
    inline float __InverseMultiQuadric__(const float& epsilon, const float& r) {
      return (1.0f / sqrt(1.0f + epsilon * epsilon * r * r));
    }    

    inline float __MultiQuadric__(const float& epsilon, const float& r) {
      return sqrt(r*r + epsilon);
    }    

#define __E_2_1 0.13533528323  // e^{-2}
    inline float __GaussianRBF__(const float& epsilon, const float& r) {
      // I {\em think} this is from reading a bunch of papers on
      // RBFs...
      const double r1 = 2.0 * epsilon * epsilon * __E_2_1;
      const float value = exp(-r * r / r1);
      return value;
    }

    inline float __ThinPlateRBF__(const float& epsilon, const float& r) {
      if (r <= 0) {
	return 0;
      }
      const float value = r * r * log(r / epsilon);
      return value;
    }

    inline RadialBasisFunction::Function InverseMultiQuadric(const float& epsilon) {
      return RadialBasisFunction::Function(epsilon, __InverseMultiQuadric__);
    }

    inline RadialBasisFunction::Function MultiQuadric(const float& epsilon) {
      return RadialBasisFunction::Function(epsilon, __MultiQuadric__);
    }

    inline RadialBasisFunction::Function GaussianRBF(const float& epsilon) {
      return RadialBasisFunction::Function(epsilon, __GaussianRBF__);
    }

    inline RadialBasisFunction::Function ThinPlateRBF(const float& epsilon) {
      return RadialBasisFunction::Function(epsilon, __ThinPlateRBF__);
    }

  }  // namespace interpolation
}  // namespace slib

#endif
