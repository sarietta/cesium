#ifndef __SLIB_UTIL_DISTANCE_H__
#define __SLIB_UTIL_DISTANCE_H__

#include "../common/types.h"
#include <math.h>

namespace slib {
  namespace util {

    template <class T> T EuclideanDistance(const T* v, const T* u, const int32& N) {
      T sum = 0;
      for (int32 i = 0; i < N; i++) {
	T d = (v[i] - u[i]);
	sum += (d*d);
      }

      return sqrt(sum);
    }

  }  // namespace util
}  // namespace slib

#endif
