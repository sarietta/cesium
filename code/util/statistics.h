#ifndef __SLIB_UTIL_STATISTICS_H__
#define __SLIB_UTIL_STATISTICS_H__

#include <common/types.h>

namespace slib {
  namespace util {

    template <class T>
    T Max(const T* data, const int32& N) {
      T max = data[0];
      
      for (int i = 0; i < N; i++) {
        if (data[i] > max) {
	  max = data[i];
	}
      }
      return max;
    }

    template <class T>
    T Min(const T* data, const int32& N) {
      T min = data[0];
      
      for (int i = 0; i < N; i++) {
        if (data[i] < min) {
	  min = data[i];
	}
      }
      return min;
    }


    template <class T>
    T Mean(const T* data, const int32& N) {
      T mean = 0;
      
      for (int i = 0; i < N; i++) {
        mean += data[i];
      }
      mean /= static_cast<T>(N);
      return mean;
    }

    template <class T>
    T Variance(const T* data, const int32& N) {
      int32 n = 0;
      T mean = 0;
      T M2 = 0;
      
      for (int i = 0; i < N; i++) {
	n = n + 1;
	T delta = data[i] - mean;
        mean += (delta / static_cast<T>(n));
        M2 += delta * (data[i] - mean); // This expression uses the new value of mean
      }
      T variance_n = M2 / static_cast<T>(n);
      T variance = M2 / static_cast<T>(n - 1);
      return variance;
    }
  }  // namespace util
}  // namespace slib

#endif
