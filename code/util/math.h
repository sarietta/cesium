#ifndef __SLIB_UTIL_MATH_H__
#define __SLIB_UTIL_MATH_H__

#include <algorithm>
#include <math.h>  // This is kind of funny... sometimes gcc gets confused and thinks we're cmath :P

namespace slib {
  namespace util {

    inline double BoundValue(const double& value, const double& opt_min, const double& opt_max) {
      double bounded_value = std::max(value, opt_min);
      bounded_value = std::min(value, opt_max);
      return bounded_value;
    }
    
    inline double DegreesToRadians(const double& deg) {
      return deg * (M_PI / 180.0);
    }
    
    inline double RadiansToDegrees(const double& rad) {
      return rad / (M_PI / 180.0);
    }
    
  }  // namespace util
}  // namespace slib

#endif
