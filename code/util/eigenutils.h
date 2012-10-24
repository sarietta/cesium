#ifndef __SLIB_UTIL_EIGENUTILS_H__
#define __SLIB_UTIL_EIGENUTILS_H__

#include <common/types.h>
#undef Success
#include <Eigen/Dense>
#include <string>

namespace slib {
  namespace util {
    class EigenUtils {
    public:
      static bool SaveMatrixToFile(const std::string& filename, const FloatMatrix& matrix);
    };
  }  // namespace util
}  // namespace slib

#endif
