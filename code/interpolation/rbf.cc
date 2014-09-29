#include "rbf.h"

#include "common/scoped_ptr.h"
#include <Eigen/Dense>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include "rbf_interp_nd.h"
#include <stdio.h>
#include <string>
#include "util/assert.h"
#include "util/matlab.h"

DEFINE_double(rbf_interpolation_radius, 0.01, "The 'radius' of the radial basis function interpolator.");

using namespace Eigen;
using slib::util::MatlabMatrix;
using std::string;

namespace slib {
  namespace interpolation {
    RadialBasisFunction::RadialBasisFunction() : _rbf(), _alt_rbf(), _condition_test(false) {}

    RadialBasisFunction::RadialBasisFunction(const Function& rbf) 
      : _rbf(rbf), _alt_rbf(), _condition_test(false) {}

    RadialBasisFunction::RadialBasisFunction(const AltFunction& rbf) 
      : _rbf(), _alt_rbf(rbf), _condition_test(false) {}

    void RadialBasisFunction::EnableConditionTest() {
      _condition_test = true;
    }

    void RadialBasisFunction::SetPoints(const MatrixXf& points) {
      _points = points;
      _N = _points.rows();
      _dimensions = _points.cols();      
    }

    void RadialBasisFunction::ComputeWeightsAlt(const VectorXf& values, const bool& normalized) {
      ASSERT_EQ(values.size(), _N);
      ASSERT_EQ(_alt_rbf.valid, true);

      const int M = _dimensions;
      const int ND = _N;
      scoped_array<double> XD(new double[M * ND]);
      for (int i = 0; i < ND; i++) {
	for (int j = 0; j < M; j++) {
	  XD[j + i * M] = (double) _points(i, j);
	}
      }

      const double R0 = FLAGS_rbf_interpolation_radius;
      scoped_array<double> FD(new double[ND]);
      for (int i = 0; i < ND; i++) {
	FD[i] = (double) values(i);
      }

      scoped_array<double> w(rbf_weight(M, ND, XD.get(), R0, _alt_rbf.function, FD.get()));

      _w.resize(ND);
      for (int i = 0; i < ND; i++) {
	_w(i) = w[i];
      }
    }

    float RadialBasisFunction::InterpolateAlt(const VectorXf& point) {
      ASSERT_EQ(point.size(), _dimensions);

      const int M = _dimensions;
      const int ND = _N;
      scoped_array<double> XD(new double[M * ND]);
      for (int i = 0; i < ND; i++) {
	for (int j = 0; j < M; j++) {
	  XD[j + i * M] = (double) _points(i, j);
	}
      }

      const double R0 = FLAGS_rbf_interpolation_radius;
      
      scoped_array<double> W(new double[ND]);
      for (int i = 0; i < ND; i++) {
	W[i] = (double) _w(i);
      }

      const int NI = 1;
      scoped_array<double> XI(new double[M * NI]);
      for (int i = 0; i < M * NI; i++) {
	XI[i] = (double) point(i);
      }

      scoped_array<double> interpolated(rbf_interp_nd(M, ND, XD.get(), R0, _alt_rbf.function, 
						      W.get(), NI, XI.get()));
      return interpolated[0];
    }

    // Input is a (w, h) = (dimensions, N) matrix of points. Values is a vector of length N.
    void RadialBasisFunction::ComputeWeights(const VectorXf& values, const bool& normalized) {
      if (!_rbf.valid && _alt_rbf.valid) {
	ComputeWeightsAlt(values, normalized);
	return;
      }

      _normalized = normalized;

      ASSERT_EQ(values.size(), _N);

      MatrixXf rbf(_N, _N);
      VectorXf b(_N);
      for (int32 i = 0; i < _N; i++) {
	float sum = 0.0f;
	for (int32 j = 0; j < _N; j++) {
	  const VectorXf difference = _points.row(i)-_points.row(j);
	  const float distance = _rbf.evaluate(difference.norm());
	  sum += distance;
	  rbf(i, j) = distance;
	}
	if (normalized) {
	  b(i) = sum * values(i);
	} else {
	  b(i) = values(i);
	}
      }
      _w = rbf.colPivHouseholderQr().solve(b);

      if (_condition_test) {
	SelfAdjointEigenSolver<MatrixXf> eigensolver(rbf);
	if (eigensolver.info() != Success) {
	  LOG(ERROR) << "Couldn't compute eigenvalues for matrix";
	} else {
	  const VectorXf eigenvalues = eigensolver.eigenvalues();
	  const float min_eig = eigenvalues(0);
	  const float max_eig = eigenvalues(eigenvalues.size() - 1);
	  const float condition_number = fabs(max_eig / min_eig);

	  LOG(INFO) << "Condition Number: " << condition_number;
	}
      }
    }

    VectorXf RadialBasisFunction::InterpolatePoints(const FloatMatrix& points) {
      VectorXf values(points.rows());
      for (int i = 0; i < points.rows(); ++i) {
	values(i) = Interpolate((VectorXf) points.row(i));
      }
      return values;
    }

    float RadialBasisFunction::Interpolate(const VectorXf& point) {
      if (!_rbf.valid && _alt_rbf.valid) {
	return InterpolateAlt(point);
      }

      ASSERT_EQ(point.size(), _dimensions);

      float weight_sum = 0.0f;
      float sum = 0.0f;
      for (int32 i = 0; i < _N; i++) {
	float value;
	if (_dimensions == 2) {
	  const float d0 = _points(i,0) - point(0);
	  const float d1 = _points(i,1) - point(1);
	  value = _rbf.evaluate(sqrt(d0*d0 + d1*d1));
	} else {
	  const VectorXf row = _points.row(i);
	  const VectorXf difference = point - row;	
	  value = _rbf.evaluate(difference.norm());
	}
	weight_sum += _w(i) * value;
	sum += value;
      }

      return _normalized ? weight_sum / sum : weight_sum;
    }

    bool RadialBasisFunction::SaveToFile(const string& filename) {
      FILE* fid = fopen(filename.c_str(), "wb");
      const int N = _N;
      const int dimensions = _dimensions;
      const int normalized = _normalized;
      int written = 0;
      written += fwrite(&N, sizeof(int), 1, fid);
      written += fwrite(&dimensions, sizeof(int), 1, fid);
      written += fwrite(&normalized, sizeof(int), 1, fid);
      if (written != 3) {
	LOG(WARNING) << "Did not write header to file: " << filename;
	return false;
      }

      // Weights.
      written = fwrite(_w.data(), sizeof(float), N, fid);
      if (written != N) {
	LOG(WARNING) << "Weights not written to file: " << filename;
	return false;
      }

      // Points.
      written = fwrite(_points.data(), sizeof(float), N * dimensions, fid);
      if (written != N*dimensions) {
	LOG(WARNING) << "Points not written to file: " << filename;
	return false;
      }

      fclose(fid);

      return true;
    }

    bool RadialBasisFunction::LoadFromFile(const string& filename) {
      FILE* fid = fopen(filename.c_str(), "rb");
      if (!fid) {
	LOG(WARNING) << "Could not open file: " << filename;
	return false;
      }
      int read = 0;

      int N, dimensions, normalized;
      read += fread(&N, sizeof(int), 1, fid);
      read += fread(&dimensions, sizeof(int), 1, fid);
      read += fread(&normalized, sizeof(int), 1, fid);
      if (read != 3) {
	LOG(WARNING) << "Could not read header from file: " << filename;
	return false;
      }
      _N = N;
      _dimensions = dimensions;
      _normalized = (bool) normalized;

      // Weights.
      _w = VectorXf(N);
      read = fread(_w.data(), sizeof(float), N, fid);
      if (read != N) {
	LOG(WARNING) << "Could not read weights from file: " << filename;
	return false;
      }

      // Points.
      _points = MatrixXf(N, dimensions);
      read = fread(_points.data(), sizeof(float), N * dimensions, fid);
      if (read != N * dimensions) {
	LOG(WARNING) << "Could not read points from file: " << filename;
	return false;
      }

      fclose(fid);
      
      return true;
    }

    MatlabMatrix RadialBasisFunction::ConvertToMatlabMatrix() const {
      LOG(ERROR) << "Not Implemented";
      return MatlabMatrix();
    }

  }  // namespace interpolation
}  // namespace slib
