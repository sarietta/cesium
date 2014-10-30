#include "fastrbf.h"

#ifdef CUDA_ENABLED
#include "fastrbf.cu.h"
#endif

#include <common/scoped_ptr.h>
#include <Eigen/Dense>
#include <float.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <stdio.h>
#include <string>
#include <util/assert.h>
#include <util/matlab.h>
#include <util/random.h>
#include <util/stl.h>
#include <util/timer.h>
#include <vector>

#define FloatVector Eigen::VectorXf
#define IntMatrix Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
#define IntVector Eigen::VectorXi

#define SYMMETRIC_LOOKUP(matrix, row, col) (row < col ? matrix(col, row) : matrix(row, col))

using slib::util::MatlabMatrix;
using slib::util::Timer;
using std::vector;

namespace slib {
  namespace interpolation {
    
    DEFINE_double(slib_interpolation_fastrbf_tolerance, 1e-4, "The error tolerance threshold.");
    DEFINE_int32(slib_interpolation_fastrbf_max_iterations, 1000, "The maximum number of iterations to perform.");
    
    FastMultiQuadraticRBF::FastMultiQuadraticRBF() : 
      _epsilon(0.0f), _epsilon2(0.0f), _power(0.0f), _alpha(0.0f) {}
    
    FastMultiQuadraticRBF::FastMultiQuadraticRBF(const float& epsilon, const int& power) : 
      _epsilon(epsilon), _power(power), _alpha(0.0f) {
      _epsilon2 = _epsilon * _epsilon;
    }
    
    void FastMultiQuadraticRBF::SetParameters(const float& epsilon, const int& power) {
      if (_w.size() != 0) {
        LOG(ERROR) << "Attempted to set RBF parameters after they've been computed. "
          "Move this call ahead of ComputeWeights()";
        return;
      }
      _epsilon = epsilon;
      _power = power;
      _epsilon2 = _epsilon * _epsilon;
    }
    
    void SetupLSet(const vector<int>& Omega, const FloatMatrix& Phi, const int& power, const int& m,
                   IntVector* Lset, vector<int>* OmegaOut, int* ell) {
      vector<int> Omega1(Omega);
      const int N = Omega1.size();
      (*ell) = Omega1[m];     
      
      int loopcount = 0;
      
      if ((N - m) > power) {
        Lset->resize(power);
        
        int flag = 1;
        
        while (flag == 1) {
          loopcount++;
          
          vector<float> dist2ell(N - m);
          for (int j = m; j < N; ++j) {
            const int jj = Omega1[j];
            dist2ell[j - m] = SYMMETRIC_LOOKUP(Phi, *ell, jj);
          }
          
          const vector<int> PermIndx = slib::util::StableSort(&dist2ell);
          for (int i = 0; i < power; ++i) {
            (*Lset)(i) = Omega1[PermIndx[i] + m];
          }
          
          float mindist2 = 0.5f * dist2ell[1];
          int minbeta = 1;
          int mingamma = 2;
          float mindist = FLT_MAX;
          
          for (int beta = 0; beta < power; ++beta) {
            const int jbeta = (*Lset)(beta);
            for (int gamma = beta + 1; gamma < power; ++gamma) {
              const int jgamma = (*Lset)(gamma);
              const float dist2betagamma = SYMMETRIC_LOOKUP(Phi, jbeta, jgamma);
              const float mindistnew = std::min(mindist, dist2betagamma);
              
              if (mindistnew != mindist) {
                minbeta = jbeta;
                mingamma = jgamma;
                mindist = mindistnew;
              }
            }
          }
          
          if (mindist < mindist2) {
            const float dist2gammaell = SYMMETRIC_LOOKUP(Phi, mingamma, *ell);
            const float dist2betaell = SYMMETRIC_LOOKUP(Phi, minbeta, *ell);
            if (dist2gammaell < dist2betaell) {
              const int t = minbeta;
              minbeta = mingamma;
              mingamma = t;
            }
            
            int mhat = 0;
            for (int i = 0; i < Omega1.size(); ++i) {
              if (Omega1[i] == minbeta) {
                mhat = i;
                break;
              }
            }
            
            (*ell) = minbeta;
            const int t = Omega1[m];
            Omega1[m] = Omega1[mhat];
            Omega1[mhat] = t;
          } else {
            flag = 0;
          }
        }
      } else {
        Lset->resize(N - m);
        for (int i = 0; i < N - m; ++i) {
          (*Lset)(i) = Omega1[m + i];
        }
      }
      
      OmegaOut->swap(Omega1);
    }
    
    FloatVector ComputeZeta(const float& c, const IntVector& Lset, const FloatMatrix& rset) {
      const int nq = Lset.size();
      FloatMatrix SysMx = FloatMatrix::Zero(nq + 1, nq + 1);
      for (int i = 0; i < nq; ++i) {
        const int i_ = Lset(i);
        const FloatVector ri = rset.col(i_);
        for (int j = i + 1; j < nq; ++j) {
          const int j_ = Lset(j);
          const FloatVector rj = rset.col(j_);
#if 0
          SysMx(i, j) = sqrt((ri - rj).dot(ri - rj) + c * c);
#else
          SysMx(i, j) = 1.0f / sqrt((ri - rj).dot(ri - rj) * c * c + 1.0f);
#endif
        }
      }
      
      SysMx.block(nq, 0, 1, nq) = FloatMatrix::Constant(1, nq, 1.0f);
      {
        const FloatMatrix t = SysMx.transpose();
        SysMx += t;
      }
      
      for (int i = 0; i < nq; ++i) {
#if 0
        SysMx(i, i) = fabs(c);
#else
        SysMx(i, i) = 1.0f;
#endif
      }
      
      FloatVector rhs = FloatVector::Zero(nq + 1);
      rhs(0, 0) = 1.0f;
      
      return SysMx.colPivHouseholderQr().solve(rhs);
    }
    
    void ComputeTau(const FloatVector& zeta, const IntVector& Lset, const FloatVector& res, const int& length,
                    FloatVector* tau) {
      float sum = 0.0f;
      
      for (int i = 0; i < length; ++i) {
        const int i_ = Lset(i);
        sum += zeta(i) * res(i_);
      }
      
      //ASSERT_TRUE(zeta(0,0) != 0.0f);
      const float myu = zeta(0, 0) == 0.0f ? sum : sum / zeta(0, 0);
      
      for (int j = 0; j < length; ++j) {
        const int j_ = Lset(j);
        (*tau)(j_) += myu * zeta(j);
      }
    }
    
    FloatMatrix ComputeDelta(const FloatVector& tau, const FloatVector& delta, const FloatVector& d) {
      const float beta = tau.dot(d) / delta.dot(d);
      return (tau - beta * delta);
    }
    
    FloatVector ComputeD(const FloatMatrix& SysMx, const FloatVector& delta) {
      const int N = delta.size();
      return (SysMx.block(0, 0, N, N) * delta);
    }
    
    void FastMultiQuadraticRBF::ComputeWeights(const FloatVector& values, const bool& normalized) {
      slib::util::Random::InitializeIfNecessary();
      
      if (FLAGS_v > 0) {
        Timer::Start();
      }
      ASSERT_EQ(values.size(), _N);
      
      const float c = _epsilon;
      const int N = values.size();
      const int power = _power;
      
      IntMatrix Lsets = IntMatrix::Zero(N, power);
      IntVector Lsetspow = IntVector::Zero(N);
      FloatMatrix zetas = FloatMatrix::Zero(N, power);
      FloatVector delta = FloatVector::Zero(N);
      
      FloatMatrix Phi = FloatMatrix::Zero(N, N);
      FloatMatrix SysMx = FloatMatrix::Zero(N + 1, N + 1);
      
      const FloatMatrix rset = _points.transpose();
      
      VLOG(1) << "Setting up matrices";
      Timer::StartIf(FLAGS_v > 0);
      for (int i = 0; i < N; ++i) {
        const FloatVector ri = rset.col(i);
        for (int j = 0; j < i; ++j) {
          const FloatVector rj = rset.col(j);
          const float r = sqrt((ri - rj).dot(ri - rj));
          Phi(j, i) = r;
#if 0
          SysMx(j, i) = sqrt(r * r + c * c);
#else
          SysMx(j, i) = 1.0f / sqrt(r * r * c * c + 1.0f);
#endif
        }
      }
#if 0      
      {
        const FloatMatrix t = Phi + Phi.transpose();
        Phi = t;
      }
#endif 
      SysMx.block(N, 0, 1, N) = FloatMatrix::Constant(1, N, 1.0f);
      {
        const FloatMatrix t = SysMx + SysMx.transpose();
        SysMx = t;
      }
      
      for (int i = 0; i < N; ++i) {
#if 0
        SysMx(i, i) = fabs(c);
#else
        SysMx(i, i) = 1.0f;
#endif
      }
      
      VLOG(1) << "Elapsed time: " << Timer::Stop();
      
      vector<int> Omega = slib::util::Random::PermutationIndices(0, N - 1);
      
      int ell;
      vector<int> ells(N - 1);
      VLOG(1) << "Initializing Krylov matrices";
      Timer::StartIf(FLAGS_v > 0);
      for (int m = 0; m < N - 1; ++m) {
        IntVector Lset;
        SetupLSet(Omega, Phi, power, m, &Lset, &Omega, &ell);
        ells[m] = ell;
        const int pow = Lset.size();
        Lsetspow(ell) = pow;
        Lsets.block(ell, 0, 1, pow) = Lset.head(pow).transpose();
        const FloatVector zeta = ComputeZeta(c, Lset, rset);
        zetas.block(ell, 0, 1, pow) = zeta.head(pow).transpose();
      }
      VLOG(1) << "Elapsed time: " << Timer::Stop();
      
      FloatVector lambdas = FloatVector::Zero(N);
      float alpha = 0.5f * (values.minCoeff() + values.maxCoeff());
      FloatVector res = values.array() - alpha;
      
      float err = std::max(fabs(res.maxCoeff()), fabs(res.minCoeff()));
      int k = 0;
      
      float min_err = err;
      _w = lambdas;
      _alpha = alpha;
      
      VLOG(1) << "** Iteration: " << k;
      VLOG(1) << "Error: " << err;
      VLOG(1) << "Residuals: " << res.head(std::min(10, (int) res.size())).transpose() << " ...";
      VLOG(1) << "Alpha: " << alpha;
      
      vector<float> errs;
      FloatVector d;
      while (err > FLAGS_slib_interpolation_fastrbf_tolerance 
             && k < FLAGS_slib_interpolation_fastrbf_max_iterations) {
        ++k;
        errs.push_back(err);
        
        FloatVector tau = FloatVector::Zero(N);	
        
        for (int m = 0; m < N - 1; ++m) {
          const int ell = ells[m];
          const int length = Lsetspow(ell);
          const FloatVector zeta = zetas.block(ell, 0, 1, length).transpose();
          IntVector Lset = Lsets.block(ell, 0, 1, length).transpose();
          VLOG(3) << m << ": " << length << ", " << ell;
          ComputeTau(zeta, Lset, res, length, &tau);
        }
        
        if (k == 1) {
          delta = tau;
        } else {
          delta = ComputeDelta(tau, delta, d);
        }
        
        d = ComputeD(SysMx, delta);
        
        const float gamma = delta.dot(res) / delta.dot(d);
        res -= (gamma * d);
        const float newconst1 = res.maxCoeff();
        const float newconst2 = res.minCoeff();
        err = std::max(fabs(newconst1), fabs(newconst2));
        const float newconst = 0.5f * (newconst1 + newconst2);
        alpha += newconst;
        res = res.array() - newconst;
        lambdas += (gamma * delta);
        
        if (k == 1 || err < min_err) {
          min_err = err;
          _w = lambdas;
          _alpha = alpha;

          VLOG(1) << "** Iteration: " << k;
          VLOG(1) << "Error: " << err;
          VLOG(1) << "Residuals: " << res.head(std::min(10, (int) res.size())).transpose() << " ...";
          VLOG(1) << "Alpha: " << alpha;
        } else {        
          VLOG(2) << "** Iteration: " << k;
          VLOG(2) << "Error: " << err;
          VLOG(2) << "Residuals: " << res.head(std::min(10, (int) res.size())).transpose() << " ...";
          VLOG(2) << "Alpha: " << alpha;
        }
      }
      
      VLOG(1) << "Elapsed time to compute weights: " << Timer::Stop();
    }
    
    FloatMatrix Replicate(const FloatVector& A, const int& N, const bool& transpose) {
      const int d = A.size();
      FloatMatrix B(N, d);
      float* data = B.data();
      const float* v = A.data();
      for (int i = 0; i < N; ++i) {
        memcpy(data + i * d, v, sizeof(float) * d);
      }
      if (transpose) {
        return B.transpose();
      } else {
        return B;
      }
    }
    
    float FastMultiQuadraticRBF::Interpolate(const FloatVector& point) {
      ASSERT_EQ(point.size(), _dimensions);
      
      float sum = 0.0f;
      for (int32 i = 0; i < _N; ++i) {
        float d;
        if (_dimensions == 2) {
          const float d0 = _points(i,0) - point(0);
          const float d1 = _points(i,1) - point(1);
          d = d0 * d0 + d1 * d1;
        } else {
          const FloatVector row = _points.row(i);
          const FloatVector difference = point - row;	
          d = difference.squaredNorm();
        }
        sum += _w(i) * sqrt(d + _epsilon2);
      }
      
      return (sum + _alpha);
    }
    
#ifdef CUDA_ENABLED
    FloatVector FastMultiQuadraticRBF::InterpolatePoints(const FloatMatrix& points) const {
      ASSERT_EQ(points.cols(), _dimensions);
      return CUDAInterpolatePoints(_points, _w, _alpha, _epsilon2, points);
    }
#endif
    
    bool FastMultiQuadraticRBF::SaveToFile(const string& filename) {
      FILE* fid = fopen(filename.c_str(), "wb");
      if (!fid) {
        LOG(ERROR) << "Could not write to file: " << filename;
        return false;
      }
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
      
      // Other parameters of the fastrbf.
      const float epsilon = _epsilon;
      const float epsilon2 = _epsilon2;
      const int power = _power;
      const float alpha = _alpha;
      written = 0;
      written += fwrite(&epsilon, sizeof(float), 1, fid);
      written += fwrite(&epsilon2, sizeof(float), 1, fid);
      written += fwrite(&power, sizeof(int), 1, fid);
      written += fwrite(&alpha, sizeof(float), 1, fid);
      if (written != 4) {
        LOG(WARNING) << "Did not write extra parameters to file: " << filename;
        return false;
      }
      
      fclose(fid);
      
      return true;
    }
    
    bool FastMultiQuadraticRBF::LoadFromFile(const string& filename) {
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
      _w = FloatVector(N);
      read = fread(_w.data(), sizeof(float), N, fid);
      if (read != N) {
        LOG(WARNING) << "Could not read weights from file: " << filename;
        return false;
      }
      
      // Points.  
      // Keeping this a 'normal' MatrixXf to preserve
      // backwards compatibility but might change it later.
      _points = Eigen::MatrixXf(N, dimensions);
      read = fread(_points.data(), sizeof(float), N * dimensions, fid);
      if (read != N * dimensions) {
        LOG(WARNING) << "Could not read points from file: " << filename;
        return false;
      }
      
      float epsilon, epsilon2, power, alpha;
      read = 0;
      read += fread(&epsilon, sizeof(float), 1, fid);
      read += fread(&epsilon2, sizeof(float), 1, fid);
      read += fread(&power, sizeof(int), 1, fid);
      read += fread(&alpha, sizeof(float), 1, fid);
      if (read != 4) {
        LOG(WARNING) << "Could not read extra parameters from file: " << filename;
        return false;
      }
      _epsilon = epsilon;
      _epsilon2 = epsilon2;
      _power = power;
      _alpha = alpha;
      
      fclose(fid);
      
      return true;
    }
    
    MatlabMatrix FastMultiQuadraticRBF::ConvertToMatlabMatrix() const {
      const MatlabMatrix N(_N);
      const MatlabMatrix dimensions(_dimensions);
      const MatlabMatrix normalized(_normalized);
      const MatlabMatrix w(_w);
      const MatlabMatrix points(_points);
      const MatlabMatrix epsilon(_epsilon);
      const MatlabMatrix epsilon2(_epsilon2);
      const MatlabMatrix power(_power);
      const MatlabMatrix alpha(_alpha);
      
      MatlabMatrix matrix(slib::util::MATLAB_STRUCT, 1, 1);
      matrix.SetStructField("N", N);
      matrix.SetStructField("dimensions", dimensions);
      matrix.SetStructField("normalized", normalized);
      matrix.SetStructField("w", w);
      matrix.SetStructField("points", points);
      matrix.SetStructField("epsilon", epsilon);
      matrix.SetStructField("epsilon2", epsilon2);
      matrix.SetStructField("power", power);
      matrix.SetStructField("alpha", alpha);
      
      return matrix;
    }
    
    void FastMultiQuadraticRBF::LoadFromMatlabMatrix(const MatlabMatrix& matrix) {
      _N = matrix.GetStructField("N").GetScalar();
      _dimensions = matrix.GetStructField("dimensions").GetScalar();
      _normalized = matrix.GetStructField("normalized").GetScalar();
      _w = matrix.GetStructField("w").GetCopiedContents();
      _points = matrix.GetStructField("points").GetCopiedContents();
      _epsilon = matrix.GetStructField("epsilon").GetScalar();
      _epsilon2 = matrix.GetStructField("epsilon2").GetScalar();
      _power = matrix.GetStructField("power").GetScalar();
      _alpha = matrix.GetStructField("alpha").GetScalar();
    }
    
  }  // namespace interpolation
}  // namespace slib
