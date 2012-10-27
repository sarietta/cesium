#ifndef __SLIB_MATH_H__
#define __SLIB_MATH_H__

// Statistics are math too :).
#include "../util/statistics.h"

template <typename T> 
int signum(T val) {
  return (T(0) < val) - (val < T(0));
}

#endif
