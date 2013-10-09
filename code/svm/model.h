#ifndef __SLIB_SVM_MODEL_H__
#define __SLIB_SVM_MODEL_H__

#include <common/scoped_ptr.h>
#include <vector>

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
#if 0
      Model(const slib::util::MatlabMatrix& libsvm_model) {
	const MatlabMatrix& SVs = libsvm_model.GetStructField("SVs");
	const Pair<int> dimensions = SVs.GetDimensions();

	const int num_SVs = dimensions.x;
	num_weights = dimensions.y;
	
	weights.reset(new float[num_weights]);
	for (int i = 0; i < num_weights; i++) {
	  weights[i] = 0.0f;
	}
	for (int j = 0; j < num_SVs; j++) {
	  int x_index = 0;
	  while (SVs(j,x_index).index != -1) {
	    const int feature_dimension_index = libmodel->SV[j][x_index].index - 1;
	    model->weights[feature_dimension_index] += libmodel->sv_coef[0][j] * libmodel->SV[j][x_index].value;
	    x_index++;
	  }
	}
	
	model->rho = libmodel->rho[0];
	model->first_label = libmodel->label[0];
      }
#endif      
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
