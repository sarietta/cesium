#ifndef __SLIB_UTIL_RANDOM_H__
#define __SLIB_UTIL_RANDOM_H__

#include <common/types.h>
#include <vector>

namespace slib {
  namespace util {

    class Random {
      static void Initialize();
      static int RandomInteger(const int& rangeStart, const int& rangeEnd);
      static uint32 RandomUnsignedInteger(const uint32& rangeStart, const uint32& rangeEnd);
      static float Uniform(const float& rangeStart, const float& rangeEnd);
      static float Gaussian();
      static float Cauchy();
      static std::vector<int> PermutationIndices(const int& start, const int& end);

    private:
      static char* rngState;

      Random();
    };

  }  // namespace util
}  // namespace slib

#endif
