#include "matlab_function.h"

#include <glog/logging.h>
#include <string>

using std::string;

namespace slib {
  namespace util {
#if 0
    MatlabFunction MatlabFunction::LoadFunction(const string& function) {
      if (!_initialized) {
	if (!mclInitializeApplication(NULL, 0)) {
	  LOG(ERROR) << "Could not initialize the application";
	  return _undefined;
	}
      }

      return new __MATLAB_FUNCTION_CREATOR(function);
    }

    void MatlabFunction::Run(const MatlabMatrix& A, MatlabMatrix* B) const {
      mxArray* data;
      RunFunction(A._matrix, &data);
      B->AssignData(data);
    }
#endif
  }  // namespace util
}  // namespace slib
