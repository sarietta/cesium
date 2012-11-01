#include "random.h"

#include <common/types.h>
#include <map>
#include <math.h>
#include <stdlib.h>
#include <vector>

using std::map;
using std::vector;

namespace slib {
  namespace util {

    char* Random::rngState = new char[256];

    void Random::initRandom() {
      initstate(4, rngState, 256);
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
	x1 = genUniformRandom(0.0, 1.0);
      } while (x1 == 0); // cannot take log of 0.
      x2 = genUniformRandom(0.0, 1.0);
      float z;
      z = sqrtf(-2.0 * logf(x1)) * cosf(2.0 * M_PI * x2);
      return z;
    }

    float Random::Cauchy() {
      float x, y;
      x = genGaussianRandom();
      y = genGaussianRandom();
      if (fabs(y) < 0.0000001) {
	y = 0.0000001;
      }
      return x / y;
    }

    vector<int> Random::PerumtationIndices(const int& start, const int& end) {
      const int num = end - start + 1;
      // Ensures that we have unique indices.
      map<int, bool> unique_indices;
      vector<int> indices;
      while (unique_indices.size() < num) {
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
