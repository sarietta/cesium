#include "random.h"

#include <common/types.h>
#undef Success
#include <Eigen/Dense>
#include <limits>
#include <map>
#include <math.h>
#include <util/matlab.h>
#include <stdlib.h>
#include <vector>

using Eigen::VectorXf;
using std::map;
using std::vector;

namespace slib {
  namespace util {

    char* Random::rngState = new char[256];
    bool Random::_initialized = false;

    void Random::InitializeIfNecessary(const int& seed) {
      if (!_initialized) {
	Initialize(seed);
      }
    }

    void Random::Initialize(const int& seed) {
      initstate(seed, rngState, 256);
      _initialized = true;
    }

    int Random::RandomInteger(const int& rangeStart, const int& rangeEnd) {
      int r;
      r = rangeStart + (int) ((rangeEnd - rangeStart + 1.0) * random() / (RAND_MAX + 1.0));
      return r;
    }

    uint32 Random::RandomUnsignedInteger(const uint32& rangeStart, const uint32& rangeEnd) {
        uint32 r;
	if (RAND_MAX >= rangeEnd - rangeStart) {
	  r = rangeStart + (uint32) ((rangeEnd - rangeStart + 1.0) * random() / (RAND_MAX + 1.0));
	} else {
	  r = rangeStart + (uint32) ((rangeEnd - rangeStart + 1.0) 
				     * ((uint64) random() * ((uint64) RAND_MAX + 1) 
					+ (uint64) random()) / ((uint64) RAND_MAX 
								* ((uint64) RAND_MAX + 1) 
								+ (uint64) RAND_MAX + 1.0));
	}
	return r;
    }

    float Random::Uniform(const float& rangeStart, const float& rangeEnd) {
      float  r;
      r = rangeStart + ((rangeEnd - rangeStart) * (float) random() / (float) RAND_MAX);
      return r;
    }

    float Random::Gaussian() {
      // Use Box-Muller transform to generate a point from normal
      // distribution.
      float x1, x2;
      do{
	x1 = Uniform(0.0, 1.0);
      } while (x1 == 0); // cannot take log of 0.
      x2 = Uniform(0.0, 1.0);
      float z;
      z = sqrtf(-2.0 * logf(x1)) * cosf(2.0 * M_PI * x2);
      return z;
    }

    float Random::Cauchy() {
      float x, y;
      x = Gaussian();
      y = Gaussian();
      if (fabs(y) < 0.0000001) {
	y = 0.0000001;
      }
      return x / y;
    }

    VectorXf Random::SampleArbitraryDistribution(const VectorXf& distribution, const int& num_samples) {
      const int size = distribution.size();

      const VectorXf normalized = distribution / distribution.sum();

      VectorXf cumulative(size);
      cumulative(0) = normalized(0);
      for (int i = 1; i < size;  i++) {
	cumulative(i) = cumulative(i - 1) + normalized(i);
      }

      vector<float> steps(size);
      for (int i = 0; i < size; i++) {
	steps[i] = ((float) i) / ((float) size - 1);
      }
      steps[size-1] = 1.0f;

      VectorXf cumulative_inverse(size);
      int index = 0;
      for (int i = 0; i < size; i++) {
	if (steps[i] + std::numeric_limits<float>::epsilon() < cumulative(index)) {
	  cumulative_inverse(i) = (float) index;
	} else {
	  while (index < size - 1
		 && steps[i] + std::numeric_limits<float>::epsilon() > cumulative(index)) {
	    index++;
	  }
	  cumulative_inverse(i) = (float) index;
	}
	// Have to stop early in some cases.
	if (index >= size) {
	  for (int j = i+1; j < size; j++) {
	    cumulative_inverse(j) = (float) size - 1;
	  }
	  break;
	}
      }
      
      VectorXf samples(num_samples);
      for (int i = 0; i < samples.size(); i++) {
	const int index = rand() % (distribution.size() - 1);
	samples(i) = cumulative_inverse(index);
      }
      
      return samples;
    }

    FloatMatrix Random::SampleArbitraryDistribution(const FloatMatrix& distribution, const int& num_samples) {
      // Compute marginal probability distribution over one of the dimensions (cols).
      const VectorXf marginal = distribution.rowwise().sum() / ((float) distribution.cols());
      
      FloatMatrix samples(num_samples, 2);
      // Sample the marginal PDF.
      const VectorXf row_samples = SampleArbitraryDistribution(marginal, num_samples);
      for (int i = 0; i < num_samples; i++) {
	const int row = row_samples(i);
	// Sample the column PDF.
	const VectorXf row_data = distribution.row(row);
	const VectorXf column_sample = SampleArbitraryDistribution(row_data, 1);
	samples(i, 0) = column_sample(0);
	samples(i, 1) = row;
      }
      
      return samples;
    }

    vector<int> Random::PermutationIndices(const int& start, const int& end) {
      const int num = end - start + 1;
      // Ensures that we have unique indices.
      map<int, bool> unique_indices;
      vector<int> indices;
      while ((int) unique_indices.size() < num) {
	const int index = RandomInteger(start, end);
	if (unique_indices.find(index) == unique_indices.end()) {
	  unique_indices[index] = true;
	  indices.push_back(index);
	}
      }

      return indices;
    }
  }  // namespace util
}  // namespace slib
