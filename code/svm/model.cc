#include "model.h"

#include <common/types.h>
#include <util/matlab.h>

namespace slib {
  namespace svm {

    Model::Model(const slib::util::MatlabMatrix& libsvm_model) {
      const SparseFloatMatrix SVs = libsvm_model.GetStructField("SVs").GetCopiedSparseContents();
      const FloatMatrix coefficients = libsvm_model.GetStructField("sv_coef").GetCopiedContents();
      
      const FloatMatrix weights_ = coefficients.transpose() * SVs;
      
      num_weights = weights_.cols();
      weights.reset(new float[num_weights]);
      for (int i = 0; i < num_weights; i++) {
	weights[i] = weights_(0, i);
      }
      
      rho = libsvm_model.GetStructField("rho").GetScalar();
      first_label = libsvm_model.GetStructField("Label").GetMatrixEntry(0,0);
    }

  }  // namespace svm
}  // namespace slib
