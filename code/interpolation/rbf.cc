#include "rbf.h"

#include "../util/assert.h"
#include <Eigen/Dense>
#include <glog/logging.h>
#include <stdio.h>
#include <string>

using namespace Eigen;
using std::string;

namespace slib {
  namespace interpolation {

    RadialBasisFunction::RadialBasisFunction(float(*rbf)(const float& r)) {
      _rbf = rbf;
    }

    void RadialBasisFunction::SetPoints(const MatrixXf& points) {
      _points = points;
      _N = _points.rows();
      _dimensions = _points.cols();      
    }

    // Input is a (w, h) = (dimensions, N) matrix of points. Values is a vector of length N.
    void RadialBasisFunction::ComputeWeights(const VectorXf& values, const bool& normalized) {
      _normalized = normalized;

      ASSERT_EQ(values.size(), _N);

      MatrixXf rbf(_N, _N);
      VectorXf b(_N);
      for (int32 i = 0; i < _N; i++) {
	float sum = 0.0f;
	for (int32 j = 0; j < _N; j++) {
	  const VectorXf difference = _points.row(i)-_points.row(j);
	  const float distance = (*_rbf)(difference.norm());
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
    }

    float RadialBasisFunction::Interpolate(const VectorXf& point) {
      ASSERT_EQ(point.size(), _dimensions);

      float weight_sum = 0.0f;
      float sum = 0.0f;
      for (int32 i = 0; i < _N; i++) {
	float value;
	if (_dimensions == 2) {
	  const float d0 = _points(i,0) - point(0);
	  const float d1 = _points(i,1) - point(1);
	  value = (*_rbf)(sqrt(d0*d0 + d1*d1));
	} else {
	  const VectorXf row = _points.row(i);
	  const VectorXf difference = point - row;	
	  value = (*_rbf)(difference.norm());
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

  }  // namespace interpolation
}  // namespace slib
