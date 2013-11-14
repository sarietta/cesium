#ifndef __SLIB_SVM_MODEL_H__
#define __SLIB_SVM_MODEL_H__

#include <common/scoped_ptr.h>
#include <common/types.h>
#include <vector>

namespace slib {
  namespace util {
    class MatlabMatrix;
  }
}

namespace slib {
  namespace svm {
    
    struct Model {
      // Size of this should mirror the response from Detector::GetFeatureDimensions.
      scoped_array<float> weights;
      int32 num_weights;
      // The offset from the origin for the SVM.
      float rho;
      float first_label;  // Don't know what this is...
      float threshold;
      
      int32 num_positives;
      int32 num_negatives;
      
      Model(const std::vector<float> weights_, const float rho_, 
	    const float first_label_, const float threshold_) {
	num_weights = weights_.size();
	weights.reset(new float[num_weights]);
	for (int32 i = 0; i < num_weights; i++) {
	  weights[i] = weights_[i];
	}
	rho = rho_;
	first_label = first_label_;
	threshold = threshold_;
      }
      
      Model(const Model& model) {
	(*this) = model;
      }
      
      Model() : weights(NULL), num_weights(0), rho(0.0f), first_label(1.0f)
	      , threshold(0.0f), num_positives(0), num_negatives(0) {}

      // Converts from a LibSVM model generated in MATLAB to a model
      // that is usable in slib.
      Model(const slib::util::MatlabMatrix& libsvm_model);

      Model& operator=(const Model& rhs) {
	num_weights = rhs.num_weights;
	weights.reset(new float[num_weights]);
	
	for (int32 i = 0; i < num_weights; i++) {
	  weights[i] = rhs.weights[i];
	}
	rho = rhs.rho;
	first_label = rhs.first_label;
	threshold = rhs.threshold;
	
	num_positives = rhs.num_positives;
	num_negatives = rhs.num_negatives;
	
	return *this;
      }            
    };
    
  }  // namespace svm
}  // namespace slib

#endif
