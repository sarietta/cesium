#ifndef __SLIB_SVM_DETECTOR_H__
#define __SLIB_SVM_DETECTOR_H__

#define SLIB_NO_DEFINE_64BIT

#include <CImg.h>
#include <common/scoped_ptr.h>
#include <common/types.h>
#undef Success
#include <Eigen/Dense>
#include <image/feature_pyramid.h>
#include <mat.h>
#include "model.h"
#include <string>
#include <vector>

namespace slib {
  namespace svm {
    
    struct DetectionParameters {
      Pair<int32> basePatchSize;
      std::vector<std::string> category;
      int32 imageCanonicalSize;
      float levelFactor;
      int32 maxClusterSize;
      int32 maxLevels;
      int32 minClusterSize;
      int32 nThNeg;
      int32 numPatchClusters;
      float overlapThreshold;
      Pair<int32> patchCanonicalSize;
      float patchOverlapThreshold;
      int32 patchScaleIntervals;
      Pair<int32> patchSize;
      int32 sBins;
      int32 scaleIntervals;
      std::string svmflags;
      float topNOverlapThresh;
      
      // TODO(sean): Should not be bools. Should be able to parse from
      // MATLAB matrix. Will remain this way for now to support legacy
      // usage.
      bool featureTypePatchOnly;
      bool featureTypeHOG;
      bool featureTypeHistogram;
      bool featureTypeSparse;
      bool featureTypeFisher;
      bool useColor;  // Whether the color channels should be added to the feature.
      
      bool selectTopN;
      int32 numToSelect;
      bool useDecisionThresh;
      float overlap;
      float fixedDecisionThresh;
      bool removeFeatures;
      float gradientSumThreshold;
      int32 interpolation_type;
      bool sampleBig;

      bool projectFeatures;
      std::string featurePCFile;
    };
    
    struct DetectionMetadata {
      int32 x1;
      int32 x2;
      int32 y1;
      int32 y2;
      
      Pair<int32> image_size;
      Triplet<int32> pyramid_offset;
      
      std::string image_path;
      
      int32 image_index;
      int32 image_set_index;

      DetectionMetadata() : x1(0), x2(0), y1(0), y2(0)
			  , image_size(0,0), pyramid_offset(0,0,0)
			  , image_path(""), image_index(0), image_set_index(0) {}
    };
    
    // A DetectionResult is a single fire of a Model.
    struct DetectionResult {
      float score;
      DetectionMetadata metadata;
    };
    
    // A ModelDetectionResultSet is a set of DetectionResults. Each Model
    // can fire multiple times per image. You can think of a
    // ModelDetectionResultSet as the set of all DetectionResults for a
    // *single* Model.
    struct ModelDetectionResultSet {
      std::vector<DetectionResult> detections;
      FloatMatrix features;
      int model_id;
    };
    
    struct DetectionResultSet {
      std::vector<ModelDetectionResultSet> model_detections;
      DetectionParameters parameters;
    };
    
    // A Detector is effectively a collection of Models.
    class Detector {
    public:
      Detector();
      virtual ~Detector() {}
      Detector(const Detector& detector);
      Detector(const DetectionParameters& parameters);

      Detector& operator=(const Detector& detector);
      
      // Computes the number of dimensions that a "feature" should
      // have. Note that this is different than the number of channels in
      // the levels of the feature pyramid.
      static int32 GetFeatureDimensions(const DetectionParameters& parameters,
					Pair<int32>* patch_size_out = NULL);
      static DetectionParameters GetDefaultDetectionParameters();
      // We allocate the memory via a deep copy. You are in charge of destroying the pointer.
      void SaveParametersToMatlabMatrix(mxArray** matrix) const;
      
      // This should be move the class FeaturePyramid and the parameters
      // should be modified accordinggly.
      static slib::image::FeaturePyramid 
      ComputeFeaturePyramid(const FloatImage& image, 
			    const DetectionParameters& parameters, 
			    const std::vector<int32>& levels = std::vector<int32>(0));

      slib::image::FeaturePyramid ComputeFeaturePyramid(const FloatImage& image, 
							const std::vector<int32>& levels = std::vector<int32>(0));
      
      // Does what it says. There is some trickery to find the offsets in
      // the correct pyramid level, etc. This is kept a public static
      // method to allow anyone to get metadata as needed.
      static std::vector<DetectionMetadata> GetDetectionMetadata(const slib::image::FeaturePyramid& pyramid,
								 const std::vector<Pair<int32> >& indices, 
								 const std::vector<int32>& selected_indices,
								 const std::vector<int32>& levels,
								 const DetectionParameters& parameters);
      
      // Outputs one DetectionResultSet per model.
      DetectionResultSet DetectInImage(const FloatImage& image);

      // A helper function that can perform the non-maximum
      // suppression required to cull a DetectionResultSet.
      static std::vector<int32> SelectViaNonMaxSuppression(const std::vector<DetectionMetadata>& metadata, 
							   const std::vector<int32>& selected_indices,
							   const Eigen::VectorXf& scores, 
							   const float& overlap_threshold);
      
      // Helper function to perform the actual prediction for all of the
      // models. If either predicted_labels or accuracy is non-NULL, then
      // the expected_labels should be provided. You can instantiate a
      // Detector and then add a single model to it, run this method, and
      // you will get the predictions for a single model.
      FloatMatrix Predict(const FloatMatrix& features, 
			  const FloatMatrix* expected_labels = NULL, 
			  FloatMatrix* predicted_labels = NULL,
			  Eigen::VectorXf* accuracy = NULL);
      
      // For adding / updating models. Both of these functions will
      // invalidate the pre-computed weight and offset matrices.
      void AddModel(const slib::svm::Model& model);
      void UpdateModel(const int32& index, const slib::svm::Model& model);

      inline void SetParameters(const DetectionParameters& parameters) {
	_parameters = parameters;
      }
      
      inline DetectionParameters GetParameters() const {
	return _parameters;
      }
      
      inline std::string GetType() const {
	return _type;
      }

      
      // These actually construct the associated matrices that are
      // normally stored in this class. Useful if you need to
      // guarantee they are up to date in a 'const' way.
      inline FloatMatrix ComputeWeightMatrix() const {
	const int32 feature_dimensions = Detector::GetFeatureDimensions(_parameters);
	FloatMatrix weight_matrix(feature_dimensions, _models.size());
	for (int j = 0; j < feature_dimensions; j++) {
	  for (uint32 i = 0; i < _models.size(); i++) {
	    weight_matrix(j, i) = _models[i].weights[j];
	  }
	}
	return weight_matrix;
      }

      inline FloatMatrix ComputeOffsetsMatrix() const {
	FloatMatrix model_offsets(1, _models.size());
	for (uint32 i = 0; i < _models.size(); i++) {
	  model_offsets(i) = _models[i].rho;
	}
	return model_offsets;
      }

      inline FloatMatrix ComputeLabelsMatrix() const {
	FloatMatrix model_labels(1, _models.size());
	for (uint32 i = 0; i < _models.size(); i++) {
	  model_labels(i) = _models[i].first_label;
	}
	return model_labels;
      }

      // Simple returns.
      inline const FloatMatrix& GetWeightMatrix() const {
	if (_weight_matrix == NULL) {
	  return _empty_matrix;
	} else {
	  return *(_weight_matrix);
	}
      }
      
      inline const FloatMatrix& GetModelOffsets(const bool& force = false) const {
	if (_model_offsets == NULL) {
	  return _empty_matrix;	 
	} else {
	  return *(_model_offsets);
	}
      }
      
      inline const FloatMatrix& GetModelLabels(const bool& force = false) const {
	if (_model_labels == NULL) {
	  return _empty_matrix;
	} else {
	  return *(_model_labels);
	}
      }

      inline int GetNumModels() const {
	return _models.size();
      }

      inline const Model& GetModel(const int& index) const {
	return _models[index];
      }
      
    private:
      DetectionParameters _parameters;
      
      std::string _type;
      std::vector<slib::svm::Model> _models;
      static FloatMatrix _empty_matrix;
      scoped_ptr<FloatMatrix> _weight_matrix;
      scoped_ptr<FloatMatrix> _model_offsets;
      scoped_ptr<FloatMatrix> _model_labels;
      
      const FloatMatrix& GetWeightMatrixInit();
      const FloatMatrix& GetModelOffsetsInit();
      const FloatMatrix& GetModelLabelsInit();
    };
    
    class DetectorFactory {
    public:
      // Caller should wrap the result in a scoped_ptr to prevent memory
      // leak. This method is not responsible for the allocated array of
      // Detectors.
      static Detector LoadFromMatlabFile(const std::string& filename);
      static Detector InitializeFromMatlabArray(const mxArray& array);
    };
    
  }  // namespace svm
}  // namespace slib

#endif
