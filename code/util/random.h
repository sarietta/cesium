#ifndef __SLIB_UTIL_RANDOM_H__
#define __SLIB_UTIL_RANDOM_H__

#include <CImg.h>
#include <common/types.h>
#undef Success
#include <Eigen/Dense>
#include <vector>

namespace slib {
  namespace util {

    class Random {
    public:
      static void Initialize(const int& seed = 4);
      static void InitializeIfNecessary(const int& seed = 4);
      static int RandomInteger(const int& rangeStart, const int& rangeEnd);
      static uint32 RandomUnsignedInteger(const uint32& rangeStart, const uint32& rangeEnd);
      static float Uniform(const float& rangeStart, const float& rangeEnd);
      static float Gaussian();
      static float Cauchy();

      // Returns a randomly permututed set of numbers from [start,
      // end]. Note that this is INCLUSIVE.
      static std::vector<int> PermutationIndices(const int& start, const int& end);

      static Eigen::VectorXf SampleArbitraryDistribution(const Eigen::VectorXf& distribution, 
							 const int& num_samples);
      static FloatMatrix SampleArbitraryDistribution(const FloatMatrix& distribution, 
						     const int& num_samples);
    private:
      static char* rngState;
      static bool _initialized;

      Random();
    };

  }  // namespace util
}  // namespace slib

#endif
