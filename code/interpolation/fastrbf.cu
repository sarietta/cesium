#include "fastrbf.cu.h"

#undef Success
#include <Eigen/Dense>

#include <common/scoped_ptr.h>
#include <common/types.h>
#include <cuda_runtime.h>
#include <glog/logging.h>
#include <stdio.h>
#include <stdlib.h>
#include "util/assert.h"

using Eigen::VectorXf;
using Eigen::MatrixXf;

namespace slib {
  namespace interpolation {
    
    __global__ 
    void Interpolate(float* interpolated, const float* points, const float* w, 
		     const int N, const float alpha, const float epsilon2,
		     const float* samples, const int num_samples) {
      const int index = threadIdx.x + blockIdx.x * blockDim.x;
      if (index < num_samples) {
	const float x = samples[2 * index + 0];
	const float y = samples[2 * index + 1];
	
	float sum = 0.0f;
	for (int32 i = 0; i < N; ++i) {
	  const float d0 = points[2 * i + 0] - x;
	  const float d1 = points[2 * i + 1] - y;
	  const float d = d0 * d0 + d1 * d1;
	  sum += w[i] * sqrt(d + epsilon2);
	}
	
	interpolated[index] = sum + alpha;
      }
    }
    
    Eigen::VectorXf CUDAInterpolatePoints(const MatrixXf& _points, const VectorXf& _w, 
					  const float& _alpha, const float& _epsilon2, 
					  const MatrixXf& _samples) {
      const int D = _samples.cols();
      const int N = _w.size();
      const int num_samples = _samples.rows();
      
      ASSERT_EQ(D, 2);
      
      scoped_array<float> points(new float[N * D]);
      scoped_array<float> w(new float[N]);
      for (int i = 0; i < N; ++i) {
	points[2 * i + 0] = _points(i, 0);
	points[2 * i + 1] = _points(i, 1);
	w[i] = _w(i);
      }
      
      scoped_array<float> samples(new float[num_samples * D]);
      for (int i = 0; i < num_samples; ++i) {
	samples[2 * i + 0] = _samples(i, 0);
	samples[2 * i + 1] = _samples(i, 1);
      }
      
      int threadsPerBlock = 256;
      int blocksPerGrid = (num_samples + threadsPerBlock - 1) / threadsPerBlock;
      VLOG(1) << "CUDA kernel launch with " << blocksPerGrid << " blocks of " << threadsPerBlock << " threads";
      
      float* d_interpolated; 
      CUDA_MALLOC(d_interpolated, sizeof(float) * num_samples);
      
      float* d_points; 
      CUDA_MALLOC(d_points, sizeof(float) * N * D);
      
      float* d_w; 
      CUDA_MALLOC(d_w, sizeof(float) * N);
      
      float* d_samples;
      CUDA_MALLOC(d_samples, sizeof(float) * num_samples * D);
      
      CUDA_UPLOAD(d_points, points.get(), sizeof(float) * N * D);
      CUDA_UPLOAD(d_w, w.get(), sizeof(float) * N);
      CUDA_UPLOAD(d_samples, samples.get(), sizeof(float) * num_samples * 2);
      
      Interpolate<<<blocksPerGrid, threadsPerBlock>>>
	(d_interpolated, d_points, d_w, N, _alpha, _epsilon2, d_samples, num_samples);
      
      // Save a little memory by copying the result into a buffer that is
      // already allocated and large enough to hold the result.
      CUDA_DOWNLOAD(samples.get(), d_interpolated, sizeof(float) * num_samples);
      
      VectorXf interpolated(num_samples);
      for (int i = 0; i < num_samples; ++i) {
	interpolated(i) = samples[i];
      }
      
      return interpolated;
    }
    
  }  // namepsace interpolation
}  // namespace slib
