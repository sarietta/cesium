#include "detector.h"

#include <algorithm>
#include <common/math.h>
#include <common/scoped_ptr.h>
#ifdef DECAF_ENABLED
#include <decaf/decaf.h>
#endif
#include <glog/logging.h>
#include <fstream>
#include <image/cimgutils.h>
#ifndef SKIP_CAFFE_FEATURE_COMPUTER
#include <image/caffe_feature_computer.h>
#endif  // SKIP_CAFFE_FEATURE_COMPUTER
#include <image/color_histogram_feature_computer.h>
#include <image/hog_feature_computer.h>
#include <image/feature_pyramid.h>
#include <iostream>
#include <mat.h>
#include <math.h>
#include <queue>
#include <string>
#include <svm/model.h>
#include <util/assert.h>
#include <util/matlab.h>
#include <util/stl.h>
#include <util/timer.h>
#include <vector>

using cimg_library::CImgList;
using Eigen::ArrayXf;
using Eigen::MatrixXf;
using Eigen::VectorXf;
using Eigen::VectorXi;
#ifndef SKIP_CAFFE_FEATURE_COMPUTER
using slib::image::CaffeFeatureComputer;
#endif  // SKIP_CAFFE_FEATURE_COMPUTER
using slib::image::ColorHistogramFeatureComputer;
using slib::image::HOGFeatureComputer;
using slib::image::FeaturePyramid;
using slib::svm::Model;
using slib::util::MatlabMatrix;
using slib::util::Timer;
using std::min;
using std::priority_queue;
using std::string;
using std::vector;

namespace slib {
  namespace svm {

    FloatMatrix Detector::_empty_matrix(0,0);
    
    Detector::Detector() 
      : _parameters(Detector::GetDefaultDetectionParameters())
      , _type("composite")
      , _weight_matrix(NULL)
      , _model_offsets(NULL)
      , _model_labels(NULL) {}
    
    Detector::Detector(const Detector& detector) 
      : _parameters(detector.GetParameters())
      , _type(detector.GetType()) {
      _models.clear();
      for (int i = 0; i < (int) detector._models.size(); i++) {
	AddModel(detector._models[i]);
      }
      _weight_matrix.reset(new FloatMatrix(detector.GetWeightMatrix()));
      _model_offsets.reset(new FloatMatrix(detector.GetModelOffsets()));
      _model_labels.reset(new FloatMatrix(detector.GetModelLabels()));
    }

    Detector& Detector::operator=(const Detector& detector) {
      _parameters = detector.GetParameters();
      _type = detector.GetType();
      _models.clear();
      for (int i = 0; i < (int) detector._models.size(); i++) {
	AddModel(detector._models[i]);
      }
      _weight_matrix.reset(new FloatMatrix(detector.GetWeightMatrix()));
      _model_offsets.reset(new FloatMatrix(detector.GetModelOffsets()));
      _model_labels.reset(new FloatMatrix(detector.GetModelLabels()));

      return (*this);
    }
    
    Detector::Detector(const DetectionParameters& parameters) 
      : _parameters(parameters)
      , _type("composite")
      , _weight_matrix(NULL)
      , _model_offsets(NULL)
      , _model_labels(NULL) {}
    
    // Helper function for the next method. Not in the class spec.
    DetectionParameters DetectorFactory::LoadParametersFromMatlabMatrix(const mxArray* params) {
      DetectionParameters parameters = Detector::GetDefaultDetectionParameters();
      mxArray* field = NULL;
      if ((field = mxGetField(params, 0, "basePatchSize"))) {
	if (mxIsDouble(field)) {
	  double* vals = mxGetPr(field);
	  parameters.basePatchSize = Pair<int32>((int32) vals[0], (int32) vals[1]);
	} else if (mxIsSingle(field)) {
	  float* vals = (float*) mxGetData(field);
	  parameters.basePatchSize = Pair<int32>((int32) vals[0], (int32) vals[1]);
	} else {
	  LOG(WARNING) << "Unknown data type for field: basePatchSize";
	}
      }
      if ((field = mxGetField(params, 0, "category"))) {
	if (mxIsCell(field)) {
	  const int num_cells = mxGetN(field) * mxGetM(field);
	  for (int i = 0; i < num_cells; i++) {
	    mxArray* cell = mxGetCell(mxGetCell(field, i), 0);
	    parameters.category.push_back(string(mxArrayToString((cell))));
	  }
	} else {
	  parameters.category.push_back(string(mxArrayToString((field))));
	}
      }
      if ((field = mxGetField(params, 0, "imageCanonicalSize"))) {
	parameters.imageCanonicalSize = (int32) mxGetScalar(field);
      }
      if ((field = mxGetField(params, 0, "levelFactor"))) {
	parameters.levelFactor = (float) mxGetScalar(field);
      }
      if ((field = mxGetField(params, 0, "maxClusterSize"))) {
	parameters.maxClusterSize = (int32) mxGetScalar(field);
      }
      if ((field = mxGetField(params, 0, "maxLevels"))) {
	parameters.maxLevels = (int32) mxGetScalar(field);
      }
      if ((field = mxGetField(params, 0, "minClusterSize"))) {
	parameters.minClusterSize = (int32) mxGetScalar(field);
      }
      if ((field = mxGetField(params, 0, "nThNeg"))) {
	parameters.nThNeg = (int32) mxGetScalar(field);
      }
      if ((field = mxGetField(params, 0, "numPatchClusters"))) {
	parameters.numPatchClusters = (int32) mxGetScalar(field);
      }
      if ((field = mxGetField(params, 0, "overlapThreshold"))) {
	parameters.overlapThreshold = (float) mxGetScalar(field);
      }
      if ((field = mxGetField(params, 0, "patchCanonicalSize"))) {
	if (mxIsDouble(field)) {
	  double* vals = mxGetPr(field);
	  parameters.patchCanonicalSize = Pair<int32>((int32) vals[0], (int32) vals[1]);
	} else if (mxIsSingle(field)) {
	  float* vals = (float*) mxGetData(field);
	  parameters.patchCanonicalSize = Pair<int32>((int32) vals[0], (int32) vals[1]);
	} else {
	  LOG(WARNING) << "Unknown data type for field: patchCanonicalSize";
	}
      }
      if ((field = mxGetField(params, 0, "patchOverlapThreshold"))) {
	parameters.patchOverlapThreshold = (float) mxGetScalar(field);
      }
      if ((field = mxGetField(params, 0, "patchScaleIntervals"))) {
	parameters.patchScaleIntervals = (int32) mxGetScalar(field);
      }
      if ((field = mxGetField(params, 0, "patchSize"))) {
	if (mxIsDouble(field)) {
	  double* vals = mxGetPr(field);
	  parameters.patchSize = Pair<int32>((int32) vals[0], (int32) vals[1]);
	} else if (mxIsSingle(field)) {
	  float* vals = (float*) mxGetData(field);
	  parameters.patchSize = Pair<int32>((int32) vals[0], (int32) vals[1]);
	} else {
	  LOG(WARNING) << "Unknown data type for field: patchSize";
	}
      }
      LOAD_PARAMETER(sBins, float);
      if ((field = mxGetField(params, 0, "scaleIntervals"))) {
	parameters.scaleIntervals = (int32) mxGetScalar(field);
      }
      if ((field = mxGetField(params, 0, "svmflags"))) {
	parameters.svmflags = string(mxArrayToString((field)));
      }
      if ((field = mxGetField(params, 0, "topNOverlapThresh"))) {
	parameters.topNOverlapThresh = (float) mxGetScalar(field);
      }
      if ((field = mxGetField(params, 0, "featureTypePatchOnly"))) {
	parameters.featureTypePatchOnly = (bool) mxGetScalar(field);
      }
      if ((field = mxGetField(params, 0, "featureTypeHOG"))) {
	parameters.featureTypeHOG = (bool) mxGetScalar(field);
      }
      if ((field = mxGetField(params, 0, "featureTypeSparse"))) {
	parameters.featureTypeSparse = (bool) mxGetScalar(field);
      }
      if ((field = mxGetField(params, 0, "featureTypeFisher"))) {
	parameters.featureTypeFisher = (bool) mxGetScalar(field);
      }
      if ((field = mxGetField(params, 0, "useColor"))) {
	parameters.useColor = (bool) mxGetScalar(field);
      }
      if ((field = mxGetField(params, 0, "sampleBig"))) {
	parameters.sampleBig = (bool) mxGetScalar(field);
      }
      if ((field = mxGetField(params, 0, "selectTopN"))) {
	parameters.selectTopN = (bool) mxGetScalar(field);
      }
      if ((field = mxGetField(params, 0, "numToSelect"))) {
	parameters.numToSelect = (int) mxGetScalar(field);
      }
      if ((field = mxGetField(params, 0, "gradientSumThreshold"))) {
	parameters.gradientSumThreshold = (float) mxGetScalar(field);
      }

      LOAD_PARAMETER(keepAllDetections, bool);

      LOAD_PARAMETER(featureTypeHistogram, bool);
      LOAD_PARAMETER(featureTypeDecaf, bool);
      LOAD_PARAMETER(featureTypeCaffe, bool);
      LOAD_PARAMETER(patchStride, int);

      LOAD_STRING_PARAMETER(decafFeatureName);

      LOAD_PARAMETER(removeDuplicateImageDetections, bool);
      LOAD_PARAMETER(removeDuplicatePanoramaDetections, bool);
      
      return parameters;
    }

    MatlabMatrix DetectorFactory::ConvertParametersToMatlabMatrix(const DetectionParameters& parameters) {
      MatlabMatrix params(slib::util::MATLAB_STRUCT, Pair<int>(1,1));

      MatlabMatrix category(slib::util::MATLAB_CELL_ARRAY, Pair<int>(1, parameters.category.size()));
      for (int i = 0; i < (int) parameters.category.size(); i++) {
	MatlabMatrix cell(slib::util::MATLAB_CELL_ARRAY, Pair<int>(1,1));
	cell.SetCell(0, MatlabMatrix(parameters.category[i]));
	category.SetCell(i, cell);
      }

      params.SetStructField("basePatchSize", 
			    MatlabMatrix(static_cast<vector<float> >(parameters.basePatchSize), false));
      params.SetStructField("category", category);
      params.SetStructField("imageCanonicalSize", 
			    MatlabMatrix(static_cast<float>(parameters.imageCanonicalSize)));
      params.SetStructField("levelFactor", MatlabMatrix(static_cast<float>(parameters.levelFactor)));
      params.SetStructField("maxClusterSize", MatlabMatrix(static_cast<float>(parameters.maxClusterSize)));
      params.SetStructField("maxLevels", MatlabMatrix(static_cast<float>(parameters.maxLevels)));
      params.SetStructField("minClusterSize", MatlabMatrix(static_cast<float>(parameters.minClusterSize)));
      params.SetStructField("nThNeg", MatlabMatrix(static_cast<float>(parameters.nThNeg)));
      params.SetStructField("numPatchClusters", MatlabMatrix(static_cast<float>(parameters.numPatchClusters)));
      params.SetStructField("overlapThreshold", MatlabMatrix(static_cast<float>(parameters.overlapThreshold)));
      params.SetStructField("patchCanonicalSize", 
			    MatlabMatrix(static_cast<vector<float> >(parameters.patchCanonicalSize), false));
      params.SetStructField("patchOverlapThreshold", 
			    MatlabMatrix(static_cast<float>(parameters.patchOverlapThreshold)));
      params.SetStructField("patchScaleIntervals", 
			    MatlabMatrix(static_cast<float>(parameters.patchScaleIntervals)));
      params.SetStructField("patchSize", 
			    MatlabMatrix(static_cast<vector<float> >(parameters.patchSize), false));
      SAVE_PARAMETER(sBins);
      params.SetStructField("scaleIntervals", MatlabMatrix(static_cast<float>(parameters.scaleIntervals)));
      params.SetStructField("svmflags", MatlabMatrix(parameters.svmflags));
      params.SetStructField("topNOverlapThresh", 
			    MatlabMatrix(static_cast<float>(parameters.topNOverlapThresh)));
      params.SetStructField("featureTypePatchOnly", 
			    MatlabMatrix(static_cast<float>(parameters.featureTypePatchOnly)));
      params.SetStructField("featureTypeHOG", 
			    MatlabMatrix(static_cast<float>(parameters.featureTypeHOG)));
      params.SetStructField("featureTypeSparse", 
			    MatlabMatrix(static_cast<float>(parameters.featureTypeSparse)));
      params.SetStructField("featureTypeFisher", 
			    MatlabMatrix(static_cast<float>(parameters.featureTypeFisher)));
      params.SetStructField("featureTypePatchOnly", 
			    MatlabMatrix(static_cast<float>(parameters.featureTypePatchOnly)));
      params.SetStructField("useColor", MatlabMatrix(static_cast<float>(parameters.useColor)));
      params.SetStructField("selectTopN", MatlabMatrix(static_cast<float>(parameters.selectTopN)));
      params.SetStructField("numToSelect", MatlabMatrix(static_cast<float>(parameters.numToSelect)));
      params.SetStructField("useDecisionThresh", MatlabMatrix(static_cast<float>(parameters.useDecisionThresh)));
      params.SetStructField("overlap", MatlabMatrix(static_cast<float>(parameters.overlap)));
      params.SetStructField("fixedDecisionThresh", 
			    MatlabMatrix(static_cast<float>(parameters.fixedDecisionThresh)));
      params.SetStructField("removeFeatures", MatlabMatrix(static_cast<float>(parameters.removeFeatures)));
      params.SetStructField("gradientSumThreshold", 
			    MatlabMatrix(static_cast<float>(parameters.gradientSumThreshold)));
      params.SetStructField("sampleBig", 
			    MatlabMatrix(static_cast<float>(parameters.sampleBig)));

      SAVE_PARAMETER(keepAllDetections);

      SAVE_PARAMETER(featureTypeHistogram);
      SAVE_PARAMETER(featureTypeDecaf);
      SAVE_PARAMETER(featureTypeCaffe);
      SAVE_PARAMETER(patchStride);

      SAVE_STRING_PARAMETER(decafFeatureName);

      SAVE_PARAMETER(removeDuplicateImageDetections);
      SAVE_PARAMETER(removeDuplicatePanoramaDetections);

      return params;
    }

    void Detector::SaveParametersToMatlabMatrix(mxArray** matrix) const {
      MatlabMatrix params = DetectorFactory::ConvertParametersToMatlabMatrix(_parameters);
      (*matrix) = mxDuplicateArray(&params.GetMatlabArray());
    }
        
    void Detector::UpdateModel(const int32& index, const Model& model) {
      // Invalidate the pre-build matrices.
      if (_weight_matrix.get()) {
	_weight_matrix.reset(NULL);
      }
      if (_model_offsets.get()) {
	_model_offsets.reset(NULL);
      }
      if (_model_labels.get()) {
	_model_labels.reset(NULL);
      }
      _models[index] = model;
    }
    
    void Detector::AddModel(const Model& model) {
      // Invalidate the pre-build matrices.
      if (_weight_matrix.get()) {
	_weight_matrix.reset(NULL);
      }
      if (_model_offsets.get()) {
	_model_offsets.reset(NULL);
      }
      if (_model_offsets.get()) {
	_model_labels.reset(NULL);
      }
      _models.push_back(model);
    }
    
    
    const FloatMatrix& Detector::GetWeightMatrixInit() {
      if (_weight_matrix.get() == NULL) {
	const int32 feature_dimensions = Detector::GetFeatureDimensions(_parameters);
	_weight_matrix.reset(new FloatMatrix(feature_dimensions, _models.size()));
	for (int j = 0; j < feature_dimensions; j++) {
	  for (uint32 i = 0; i < _models.size(); i++) {
	    ASSERT_EQ(_models[i].num_weights, feature_dimensions);
	    ASSERT_NOT_NULL(_models[i].weights);
	    (*_weight_matrix)(j, i) = _models[i].weights[j];
	  }
	}
      }
      
      return *(_weight_matrix);
    }
    
    const FloatMatrix& Detector::GetModelOffsetsInit() {
      if (_model_offsets.get() == NULL) {
	_model_offsets.reset(new FloatMatrix(1, _models.size()));
	for (uint32 i = 0; i < _models.size(); i++) {
	  (*_model_offsets)(i) = _models[i].rho;
	}
      } 
      
      return *(_model_offsets);
    }
    
    const FloatMatrix& Detector::GetModelLabelsInit() {
      if (_model_labels.get() == NULL) {
	_model_labels.reset(new FloatMatrix(1, _models.size()));
	for (uint32 i = 0; i < _models.size(); i++) {
	  (*_model_labels)(i) = _models[i].first_label;
	}
      } 
      
      return *(_model_labels);
    }
    
    struct ComponentwiseThresholdFunctor {
      float threshold;
      ComponentwiseThresholdFunctor(const float& threshold_) : threshold(threshold_) {}
      float operator()(const float& x) const {
	return (x >= threshold) ? 1.0f : 0.0f;
      }
    };
    
    struct IndexedValue {
      Pair<int32> index;
      float value;
    };
    
    struct IndexedCompareLessThan {
      bool operator() (const IndexedValue& lhs, const IndexedValue&rhs) const {
	return (lhs.value < rhs.value);
      }
    };
    
    struct IndexedCompareGreaterThan {
      bool operator() (const IndexedValue& lhs, const IndexedValue&rhs) const {
	return (lhs.value > rhs.value);
      }
    };

    Triplet<int> Detector::ImagePointToPyramidLocation(const FeaturePyramid& pyramid, 
						       const Pair<Pair<float> >& point,
						       const DetectionParameters& parameters) {
      const float x1 = point.first().x;
      const float y1 = point.first().y;
      const float x2 = point.second().x;
      const float y2 = point.second().y;

      const float width = x2 - x1;
      const float height = y2 - y1;
      const float area = width * height;

      Pair<float> patch_size;
      Detector::GetFeatureDimensions(parameters, &patch_size);      
      const float sbins = parameters.sBins;     

      // This is NOT exact. It tries to find the closest size patches
      // in the pyramid wrt to the input patch. However, because
      // patches can be cropped, this may not be the actual pyramid
      // location.
      const int num_levels = pyramid.GetNumLevels();
      int level = 0;
      float diff = -1;
      for (int i = 0; i < num_levels; i++) {
	const Pair<Pair<float> > level_patch = pyramid.GetPatchSizeInLevel(patch_size, i, sbins);
	const float x1 = level_patch.first().x;
	const float y1 = level_patch.first().y;
	const float x2 = level_patch.second().x;
	const float y2 = level_patch.second().y;
	
	const float level_patch_width = x2 - x1;
	const float level_patch_height = y2 - y1;	
	
	const float level_patch_area = level_patch_width * level_patch_height;

	const float level_diff = (level_patch_area - area) * (level_patch_area - area);
	if (i == 0 || level_diff < diff) {
	  level = i;
	  diff = level_diff;
	}
      }

      const Pair<int> pyramid_point = ImagePointToPyramidPoint(pyramid, Pair<int>(x1, y1), level, parameters);
      return Triplet<int>(level, pyramid_point.x, pyramid_point.y);
    }
    
    Pair<int> Detector::ImagePointToPyramidPoint(const FeaturePyramid& pyramid, 
						 const Pair<int>& point, const int& level, 
						 const DetectionParameters& parameters) {
      const float canonical_scale = pyramid.GetCanonicalScale();

      Pair<float> patch_size;
      Detector::GetFeatureDimensions(parameters, &patch_size);
      
      const float sbins = parameters.sBins;
      const Pair<Pair<float> > levelPatch = pyramid.GetPatchSizeInLevel(patch_size, level, sbins);
	
      const float level_scale = pyramid.GetScales()[level];

      const float xoffset = point.x - levelPatch.first().x;
      const float yoffset = point.y - levelPatch.first().y;
      
      const float x1 = xoffset * canonical_scale / (level_scale *  sbins);
      const float y1 = yoffset * canonical_scale / (level_scale *  sbins);

      return Pair<int>(x1, y1);
    }

    // Mirrors pyridx2pos.m
    Pair<int> Detector::PyramidPointToImagePoint(const float& level_scale, const float& canonical_scale,
						 const Pair<int>& point, const int& level, 
						 const DetectionParameters& parameters) {
      Pair<float> patch_size;
      Detector::GetFeatureDimensions(parameters, &patch_size);
      
      const float sbins = parameters.sBins;
      const Pair<Pair<float> > levelPatch 
	= FeaturePyramid::GetPatchSizeInLevel(patch_size, level_scale, canonical_scale, sbins);
	
      const float x1 = point.x;
      const float y1 = point.y;
      const float xoffset = floor(x1 * sbins * level_scale / canonical_scale);
      const float yoffset = floor(y1 * sbins * level_scale / canonical_scale);
      
      Pair<int> image_point;
      image_point.x = levelPatch.first().x + xoffset;
      image_point.y = levelPatch.first().y + yoffset;

      return image_point;
    }

    Pair<int> Detector::PyramidPointToImagePoint(const FeaturePyramid& pyramid, 
						 const Pair<int>& point, const int& level, 
						 const DetectionParameters& parameters) {
      const float canonical_scale = pyramid.GetCanonicalScale();
      const float level_scale = pyramid.GetScales()[level];

      return Detector::PyramidPointToImagePoint(level_scale, canonical_scale, point, level, parameters);
    }

    int32 Detector::GetFeatureDimensions(const DetectionParameters& parameters,
					 Pair<float>* patch_size_out) {
      Pair<float> patch_size;
      patch_size.x = round(((float) parameters.patchCanonicalSize.x) / parameters.sBins) - 2;
      patch_size.y = round(((float) parameters.patchCanonicalSize.y) / parameters.sBins) - 2;
      
      int32 extra_dimensions = 0;
      int32 patch_channels = 0;

      if (parameters.featureTypeHOG) {
	patch_channels = HOGFeatureComputer::GetPatchChannels();
	if (parameters.useColor) {
	  patch_channels += 2;
	}
      } else if (parameters.featureTypeHistogram) {
	patch_channels = ColorHistogramFeatureComputer::GetPatchChannels();
      } else if (parameters.featureTypeSparse) {
	// TODO(sean): IMPLEMENT ME
      } else if (parameters.featureTypeFisher) {
	// TODO(sean): IMPLEMENT ME
      } else if (parameters.featureTypePatchOnly) {
	patch_channels = 1;
      } else if (parameters.featureTypeDecaf) {
	// TODO(sean): Oh come on. This is just silly. All of it.
	patch_channels = parameters.useColor ? -2 : 0;
	if (parameters.decafFeatureName == "fc6_cudanet_out") {
	  extra_dimensions = 4096;
	} else if (parameters.decafFeatureName == "pool5_cudanet_out") {
	  extra_dimensions = 9216;
	}
      } else if (parameters.featureTypeCaffe) {
#ifdef SKIP_CAFFE_FEATURE_COMPUTER
	LOG(ERROR) << "Attempted to use Caffe feature computer, but it was not enabled. Falling back to HOG.";
	patch_channels = 31;

	if (parameters.useColor) {
	  patch_channels += 2;
	}
#else
	// Typical bins: 
	//	conv1 :: 4
	//	conv5 :: 16
	//	pool5 :: 32
	const int bins = CaffeFeatureComputer::GetBinsForNet(CaffeFeatureComputer::GetInstance()->GetNet());

	// patch_size = patchCanonicalSize / bins - 2
	patch_size = CaffeFeatureComputer::GetPatchSize(parameters.patchCanonicalSize, bins);
	patch_channels = CaffeFeatureComputer::GetPatchChannels();
#endif
      }
      
      if (patch_size_out) {
	patch_size_out->x = patch_size.x;
	patch_size_out->y = patch_size.y;
      }
      
      return (round(patch_size.x) * round(patch_size.y) * patch_channels + extra_dimensions);
    }
    
    vector<DetectionMetadata> Detector::GetDetectionMetadata(const FeaturePyramid& pyramid, 
							     const vector<Pair<int32> >& indices,
							     const vector<int32>& selected_indices,
							     const vector<int32>& levels,
							     const DetectionParameters& parameters) {
      vector<DetectionMetadata> metadata;
      const float canonical_scale = pyramid.GetCanonicalScale();
      
      Pair<float> patch_size;
      Detector::GetFeatureDimensions(parameters, &patch_size);
      
      const float sbins = parameters.sBins;
      for (uint32 i = 0; i < selected_indices.size(); i++) {
	const int32 selected_index = selected_indices[i];
	ASSERT_LT((uint32) selected_index, levels.size());
	const int32 level = levels[selected_index];
	
	const Pair<Pair<float> > levelPatch = pyramid.GetPatchSizeInLevel(patch_size, level, sbins);
	
	const float level_scale = pyramid.GetScales()[level];
	const float x1 = indices[selected_index].x;
	const float y1 = indices[selected_index].y;
	const float xoffset = floor(x1 * sbins * level_scale / canonical_scale);
	const float yoffset = floor(y1 * sbins * level_scale / canonical_scale);
	
	DetectionMetadata selection_metadata;
	selection_metadata.x1 = levelPatch.first().x + xoffset;
	selection_metadata.y1 = levelPatch.first().y + yoffset;
	selection_metadata.x2 = levelPatch.second().x + xoffset;
	selection_metadata.y2 = levelPatch.second().y + yoffset;
	
	//selection_metadata.image_size = pyramid.GetOriginalImageSize();
	selection_metadata.pyramid_offset 
	  = Triplet<int32>(level, indices[selected_index].x, indices[selected_index].y);
	
	// Clip the metadata about the patch to the boundary of the image.
	const Pair<int32> image_size = pyramid.GetOriginalImageSize();
	if (selection_metadata.x1 < 0 
	    || selection_metadata.y1 < 0
	    || selection_metadata.x2 > image_size.x - 1 
	    || selection_metadata.y2 > image_size.y - 1) {
	  selection_metadata.cropped = true;
	} else {
	  selection_metadata.cropped = false;
	}

	selection_metadata.x1 = selection_metadata.x1 < 0 ? 0 : selection_metadata.x1;
	selection_metadata.y1 = selection_metadata.y1 < 0 ? 0 : selection_metadata.y1;
	selection_metadata.x2 = selection_metadata.x2 > image_size.x - 1 ? image_size.x - 1 : selection_metadata.x2;
	selection_metadata.y2 = selection_metadata.y2 > image_size.y - 1 ? image_size.y - 1 : selection_metadata.y2;
	
	selection_metadata.image_size = image_size;

	metadata.push_back(selection_metadata);
      }
      
      return metadata;
    }

    vector<int32> Detector::SelectViaNonMaxSuppression(const vector<DetectionMetadata>& metadata, 
						       const vector<int32>& selected_indices,
						       const VectorXf& scores, 
						       const float& overlap_threshold) {
      vector<int32> indices;
      
      FloatMatrix boxes(metadata.size(), 4);
      vector<IndexedValue> sorted_indices;
      for (uint32 k = 0; k < selected_indices.size(); k++) {
	boxes(k, 0) = metadata[k].x1;
	boxes(k, 1) = metadata[k].y1;
	boxes(k, 2) = metadata[k].x2;
	boxes(k, 3) = metadata[k].y2;
	
	IndexedValue iv;
	iv.index = Pair<int32>(k, 0);
	iv.value = scores(selected_indices[k]);
	sorted_indices.push_back(iv);
      }
      
      const VectorXf& x1 = boxes.col(0);
      const VectorXf& y1 = boxes.col(1);
      const VectorXf& x2 = boxes.col(2);
      const VectorXf& y2 = boxes.col(3);
      
      const ArrayXf area = ((x2 - x1).array() + 1.0f) * ((y2 - y1).array() + 1.0f);
      
      sort(sorted_indices.begin(), sorted_indices.end(), IndexedCompareLessThan());
      
      while (sorted_indices.size() > 0) {
	if (sorted_indices.size() == 1) {
	  break;
	}
	
	const uint32 last = sorted_indices.size() - 1;
	const int32 i = sorted_indices[last].index.x;
	indices.push_back(i);
	
	MatrixXf total_set(last, 4);
	for (uint32 j = 0; j < last; j++) {
	  total_set(j, 0) = (float) x1(sorted_indices[j].index.x);
	  total_set(j, 1) = (float) y1(sorted_indices[j].index.x);
	  total_set(j, 2) = (float) x2(sorted_indices[j].index.x);
	  total_set(j, 3) = (float) y2(sorted_indices[j].index.x);
	}
	
	ArrayXf i_set(total_set.rows());
	
	i_set.fill(x1(i));
	const ArrayXf xx1 = i_set.max(total_set.col(0).array());
	
	i_set.fill(y1(i));
	const ArrayXf yy1 = i_set.max(total_set.col(1).array());
	
	i_set.fill(x2(i));
	const ArrayXf xx2 = i_set.min(total_set.col(2).array());
	
	i_set.fill(y2(i));
	const ArrayXf yy2 = i_set.min(total_set.col(3).array());
	
	i_set.fill(0.0f);
	const ArrayXf w = i_set.max(xx2 - xx1 + 1.0f).array();
	const ArrayXf h = i_set.max(yy2 - yy1 + 1.0f).array();
	
	vector<int> to_delete;
	for (int j = 0; j < w.size(); j++) {
	  const float overlap = w(j) * h(j) / area(sorted_indices[j].index.x);
	  if (overlap > overlap_threshold) {
	    to_delete.push_back(j);
	  }
	}
	for (uint32 j = 0; j < to_delete.size(); j++) {
	  sorted_indices.erase(sorted_indices.begin() + to_delete[j] - j);  // Valid because they are sorted.
	}
	sorted_indices.pop_back();
      }
      // I think this is here in case the last element in the list is not
      //connected to anyone. But I changed the above routine to be smarter
      //about it.  
      
      //indices.pop_back();
      
      return indices;

    }
    
    DetectionResultSet Detector::DetectInImage(const FloatImage& image, const string& filename) {
      DetectionResultSet result_set;
      
      FloatMatrix features;
      vector<int32> levels;
      vector<Pair<int32> > indices;
      // Compute the features for the input image.
      Timer::Start();
      // Compute the feature pyramid for the image
      scoped_ptr<FeaturePyramid> pyramid;  // Ignored for decaf features

      if (_parameters.featureTypeDecaf) {
#ifdef DECAF_ENABLED
	if (image.spectrum() != 3) {
	  LOG(INFO) << "Can't compute decaf features on non-RGB image: " << filename;
	  return result_set;
	}

	vector<DetectionMetadata> patches;
	features = Decaf::ExtractAllFeatures(image, filename, _parameters, &patches);

	for (int i = 0; i < (int) patches.size(); i++) {
	  indices.push_back(Pair<int32>(patches[i].x1, patches[i].y1));
	  levels.push_back(patches[i].pyramid_offset.first());
	}
#else
	LOG(ERROR) << "Attempted to use Decaf features but they're not enabled!";
	return result_set;
#endif
      } else {
	if (_parameters.featureTypeCaffe) {
#ifdef SKIP_CAFFE_FEATURE_COMPUTER
	  LOG(ERROR) << "Attempted to use Caffe feature computer, but it was not enabled. Falling back to HOG.";
	  pyramid.reset(new FeaturePyramid(ComputeFeaturePyramid(image)));
#else
	  CaffeFeatureComputer* computer = CaffeFeatureComputer::GetInstance();
	  pyramid.reset(new FeaturePyramid(computer->ComputeFeaturePyramid(image, 
									   _parameters.imageCanonicalSize, 
									   _parameters.scaleIntervals,
									   _parameters.patchCanonicalSize)));
#endif  // SKIP_CAFFE_FEATURE_COMPUTER
	} else if (_parameters.featureTypeHistogram) {
	  ColorHistogramFeatureComputer computer(_parameters.sBins);
	  pyramid.reset(new FeaturePyramid(computer.ComputeFeaturePyramid(image, 
									  _parameters.imageCanonicalSize, 
									  _parameters.scaleIntervals,
									  _parameters.patchCanonicalSize)));
	} else {
	  pyramid.reset(new FeaturePyramid(ComputeFeaturePyramid(image)));
	}
	
	// Get all of the features for the image.
	vector<float> gradient_sums;
	Pair<float> patch_size;
	const int32 feature_dimensions = Detector::GetFeatureDimensions(_parameters, &patch_size);
	FloatMatrix allfeatures = pyramid->GetAllLevelFeatureVectors(patch_size, feature_dimensions, 
								     &levels, &indices, &gradient_sums);
	VLOG(1) << "Number of features before gradient thresholding: " << allfeatures.rows();
	ASSERT_EQ(allfeatures.rows(), levels.size());
	ASSERT_EQ(allfeatures.rows(), levels.size());
	features = FeaturePyramid::ThresholdFeatures(allfeatures, gradient_sums, _parameters.gradientSumThreshold, 
						     &levels, &indices);
	ASSERT_EQ(features.rows(), levels.size());
	ASSERT_EQ(features.rows(), indices.size());
      }

      // Run the models against the features.
      if (_type != "composite") {
	LOG(ERROR) << "Only composite detectors are supported.";
	return result_set;
      }
      LOG(INFO) << "Elapsed time to compute feature pyramid: " << Timer::Stop();
      
      Timer::Start();  
      /* ============== Compute Detections ========================
	 detections is an N x M matrix, where N is the number of
	 features in the image and M is the number of models in the
	 detector.
       */
      VLOG(1) << "Number of features: " << features.rows();
      VLOG(1) << "Number of models: " << _models.size();
      FloatMatrix detections = Predict(features);
      LOG(INFO) << "Elapsed time to compute detections: " << Timer::Stop();
      
      // Determine which detections will be selected.
      FloatMatrix selected;
      // Only selects the top N.
      if (_parameters.selectTopN) {
	selected.resize(detections.rows(), detections.cols());
	selected.fill(0.0f);  // "false"

	VLOG(1) << "Selecting top " << _parameters.numToSelect;

	for (int i = 0; i < detections.cols(); i++) {
	  vector<float> scores(detections.rows());
	  for (int j = 0; j < scores.size(); j++) {
	    scores[j] = detections(j, i);
	  }
	  const vector<int> indices = slib::util::Sort(&scores);

	  for (int j = 0; j < min((int) scores.size(), _parameters.numToSelect); j++) {
	    selected(indices[indices.size() - 1 - j], i) = 1.0f;  // true
	  }
	}
      } else if (_parameters.useDecisionThresh) {  // Select wrt threshold
	selected = detections.unaryExpr(ComponentwiseThresholdFunctor(_parameters.fixedDecisionThresh));
      } else {  // No threshold.
	selected = detections.unaryExpr(ComponentwiseThresholdFunctor(0.0f));
      }     

      // Construct the result set per model.
      Timer::Start();
      for (int32 i = 0; i < detections.cols(); i++) {
	vector<int32> selected_indices;
	for (int j = 0; j < selected.rows(); j++) {
	  if (selected(j, i) > 0.0f) {
	    selected_indices.push_back(j);
	  }
	}
	VLOG(1) << "Initial number of detections: " << selected_indices.size();

	ModelDetectionResultSet model_result_set;
	if (selected_indices.size() < 1) {
	  //continue;
	} else {	
	  Timer::Start();
	  vector<DetectionMetadata> metadata;

	  if (_parameters.featureTypeDecaf) {
#ifdef DECAF_ENABLED
	    const float canonical_size = static_cast<float>(_parameters.imageCanonicalSize);
	    float scale = 0.0f;
	    if (image.width() < image.height()) {
	      scale = canonical_size / static_cast<float>(image.width());
	    } else {
	      scale = canonical_size / static_cast<float>(image.height());
	    }
	    
	    if (_parameters.imageCanonicalSize <= 0) {
	      scale = 1.0f;
	    }

	    const float scaled_height = (int) (((float) image.height()) * scale);
	    const float scaled_width = (int) (((float) image.width()) * scale);

	    vector<float> scales;
	    Decaf::ComputeScales(scaled_width, scaled_height, _parameters, &scales);

	    metadata.resize(selected_indices.size());
	    for (int i = 0; i < (int) selected_indices.size(); i++) {
	      const int selected_index = selected_indices[i];

	      const int level = levels[selected_index];
	      const float level_scale = scale / scales[level];
	      
	      metadata[i].x1 = indices[selected_index].x / level_scale;
	      metadata[i].y1 = indices[selected_index].y / level_scale;
	      metadata[i].x2 = metadata[i].x1 + _parameters.basePatchSize.x / level_scale;
	      metadata[i].y2 = metadata[i].y1 + _parameters.basePatchSize.y / level_scale;
	      
	      metadata[i].x1 = metadata[i].x1 < 0 ? 0 : metadata[i].x1;
	      metadata[i].y1 = metadata[i].y1 < 0 ? 0 : metadata[i].y1;
	      metadata[i].x2 = metadata[i].x2 > image.width() - 1 ? image.width() - 1 : metadata[i].x2;
	      metadata[i].y2 = metadata[i].y2 > image.height() - 1 ? image.height() - 1 : metadata[i].y2;

	      metadata[i].pyramid_offset 
		= Triplet<int32>(level, indices[selected_index].x, indices[selected_index].y);

	      metadata[i].image_size = Pair<int32>(image.width(), image.height());
	    }
#else
	    LOG(ERROR) << "Attempted to use Decaf features but they're not enabled!";
	    return result_set;
#endif
	  } else {
	    metadata = Detector::GetDetectionMetadata((*pyramid.get()), indices, selected_indices, 
						      levels, _parameters);
	  }
	  VLOG(2) << "Time spent collating the metadata: " << Timer::Stop();
	  
	  vector<int32> final_indices;
	  if (_parameters.keepAllDetections) {
	    final_indices = slib::util::Range(0, (int) selected_indices.size());
	  } else {
	    Timer::Start();
	    final_indices = SelectViaNonMaxSuppression(metadata, selected_indices, 
						       detections.col(i), _parameters.overlap);
	    VLOG(2) << "Time spent performing non-maximum suppression: " << Timer::Stop();
	  }
	  
	  VLOG(1) << "Final number of detections: " << final_indices.size();

	  model_result_set.model_id = i;
	  if (!_parameters.removeFeatures) {
	    model_result_set.features.resize(final_indices.size(), features.cols());
	  }
	  for (uint32 k = 0; k < final_indices.size(); k++) {
	    const int32 selected_index = final_indices[k];
	    DetectionResult result;
	    result.metadata = metadata[selected_index];
	    result.score = detections(selected_indices[selected_index], i);      
	    model_result_set.detections.push_back(result);
	    
	    if (!_parameters.removeFeatures) {
	      model_result_set.features.row(k) = features.row(selected_indices[selected_index]);
	    }
	  }
	}
	result_set.model_detections.push_back(model_result_set);
      }
      LOG(INFO) << "Elapsed time to construct final result set: " << Timer::Stop();
      
      return result_set;
    }
    
    FloatMatrix Detector::Predict(const FloatMatrix& features, 
				  const FloatMatrix* expected_labels,
				  FloatMatrix* predicted_labels,
				  VectorXf* accuracy) {
      // Offsets have to be computed per-image since the number of
      // features per-image may change.
      const FloatMatrix offsets = GetModelOffsetsInit().colwise().replicate(features.rows());
      const FloatMatrix model_labels = GetModelLabelsInit().colwise().replicate(features.rows());
      const FloatMatrix& weights = GetWeightMatrixInit();
      
      const FloatMatrix decisions = model_labels.cwiseProduct(features * weights - offsets);
      
      if (predicted_labels || accuracy) {
	scoped_array<int32> num_correct(new int32[_models.size()]);
	for (uint32 i = 0; i < _models.size(); i++) {
	  num_correct[i] = 0;
	}
	for (int i = 0; i < decisions.rows(); i++) {
	  for (int j = 0; j < decisions.cols(); j++) {
	    // Compute the sign of the decision (which gives us the label).
	    int sign = signum<float>(decisions(i, j));
	    char predicted_label = (sign > 0) ? +1 : -1;
	    if (predicted_labels) {
	      (*predicted_labels)(i, j) = predicted_label;
	    }
	    // WARNING: If accuracy or predicted labels are requested, then
	    // you must provide the expected labels.
	    if ((*expected_labels)(i, j) == predicted_label) {
	      num_correct[j]++;
	    }
	  }
	}
	// Compute the accuracy if requested.
	if (accuracy) {
	  for (uint32 i = 0; i < _models.size(); i++) {
	    (*accuracy)(i) = (100.0f * ((float) num_correct[i]) / ((float) features.rows()));
	  }
	}
      }
      
      return decisions;
    }
    
    DetectionParameters Detector::GetDefaultDetectionParameters() {
      DetectionParameters parameters;
      
      parameters.basePatchSize = Pair<int32>(80, 80);
      parameters.category.push_back("");
      parameters.imageCanonicalSize = 400;
      parameters.levelFactor = 2.0f;
      parameters.maxClusterSize = 10;
      parameters.maxLevels = 4;
      parameters.minClusterSize = 3;
      parameters.nThNeg = 10;
      parameters.numPatchClusters = 1000;
      parameters.overlapThreshold = 0.4000;
      parameters.patchCanonicalSize = Pair<int32>(80, 80);
      parameters.patchOverlapThreshold = 0.6000;
      parameters.patchScaleIntervals = 2;
      parameters.patchSize = Pair<int32>(80, 80);
      parameters.sBins = 8.0f;
      parameters.scaleIntervals = 8;
      parameters.svmflags = string("-s 0 -t 0 -c 0.1");
      parameters.topNOverlapThresh = 0.5000;
      parameters.featureTypePatchOnly = false;
      parameters.featureTypeHOG = true;
      parameters.featureTypeHistogram = false;
      parameters.featureTypeSparse = false;
      parameters.featureTypeFisher = false;
      parameters.featureTypeDecaf = false;
      parameters.featureTypeCaffe = false;
      parameters.decafFeatureName = "fc6_cudanet_out";
      parameters.patchStride = parameters.basePatchSize.x / 5;
      parameters.useColor = true;
      parameters.selectTopN = false;
      parameters.numToSelect = 0;
      parameters.keepAllDetections = false;
      parameters.useDecisionThresh = true;
      parameters.overlap = parameters.overlapThreshold;
      parameters.fixedDecisionThresh = -1.002;
      parameters.removeFeatures = false;
      parameters.sampleBig = false;      
      parameters.removeDuplicateImageDetections = false;
      parameters.removeDuplicatePanoramaDetections = false;

      parameters.gradientSumThreshold = 9.0f;
      
      // See:
      // http://cimg.sourceforge.net/reference/structcimg__library_1_1CImg.html#a6a668c8b3f9d756264d1fb31b7a915fc
      parameters.interpolation_type = 5;  // bicubic
      
      // Determine whether the computation of the feature pyramid
      // involves projecting the raw features to a restricted
      // subspace.
      parameters.projectFeatures = false;
      parameters.featurePCFile = "";

      return parameters;
    }

    FeaturePyramid Detector::ComputeFeaturePyramid(const FloatImage& image, 
						   const vector<int32>& levels) {
      return Detector::ComputeFeaturePyramid(image, _parameters, levels);
    }
    
    FeaturePyramid Detector::ComputeFeaturePyramid(const FloatImage& image, 
						   const DetectionParameters& parameters, 
						   const vector<int32>& levels) {
      VLOG(1) << "Original Image Size: " << image.width() << " x " << image.height();
#if 1
      // Image should have pixels in [0,1]
      if (image.max() > 1.0f) {
	LOG(ERROR) << "Image pixels must lie in the domain [0,1] (Max: " << image.max() << ")";
	return FeaturePyramid(0);
      }
#endif
            
      // Determine number of pyramid levels for the image.
      const float canonical_size = static_cast<float>(parameters.imageCanonicalSize);
      float scale = 0.0f;
      if (image.width() < image.height()) {
	scale = canonical_size / static_cast<float>(image.width());
      } else {
	scale = canonical_size / static_cast<float>(image.height());
      }

      if (parameters.imageCanonicalSize <= 0) {
	scale = 1.0f;
      }

      VLOG(1) << "Canonical Scale: " << scale;
      
      const float scaled_width = scale * image.width();
      const float scaled_height = scale * image.height();
      
      const int32 level1 
	= (int32) floor((float) parameters.scaleIntervals 
			* log2(scaled_width / ((float) parameters.basePatchSize.x)));
      const int32 level2 
	= (int32) floor((float) parameters.scaleIntervals 
			* log2(scaled_height / ((float) parameters.basePatchSize.y)));
      const int32 num_levels = min(level1, level2) + 1;
      VLOG(1) << "Number of levels in image: " << num_levels;
      
      vector<int32> levels_to_compute(levels);
      if (levels_to_compute.size() == 0) {
	for (int i = 0; i < num_levels; i++) {
	  levels_to_compute.push_back(i);
	}
      }
      
      // Determine the number of scales for the image at each level.
      const float sc = pow(2.0f, 1.0f / ((float) parameters.scaleIntervals));
      scoped_array<float> scales(new float[num_levels]);
      for (int i = 0; i < num_levels; i++) {
	scales[i] = pow(sc, (float) i);
	VLOG(2) << "Scale for level " << i << ": " << scales[i];
      }
#if 0
      const int32 num_bins = 11;
      scoped_array<int32> bins(new int32[num_bins]);
      for (int i = 0; i < num_bins; i++) {
	bins[i] = -100 + i*20;
	if (i == num_bins - 1) {
	  bins[i]++;
	}
      }
#endif 
      ASSERT_LTE((int32) levels_to_compute.size(), num_levels);

      // Convert the image to the Lab color space so it can be used as
      // necessary.
      FloatImage lab_image(image * 255);  // Conversions expect [0, 255]
      lab_image.RGBtoLab();
      lab_image *= 0.0025; 
      
      FeaturePyramid pyramid(num_levels);
      int32 numx;
      int32 numy;
      const int32 sBins = (int32) parameters.sBins;

      // Compute feature for each level in the feature pyramid.
      for (uint32 i = 0; i < levels_to_compute.size(); i++) {
	const int32 level = levels_to_compute[i];
	const float level_scale = scale / scales[level];
	VLOG(1) << "Scale: " << scales[level];
	VLOG(1) << "Level Scale: " << level_scale;
	
	const float image_level_width = ceil(level_scale * ((float) image.width()));
	const float image_level_height = ceil(level_scale * ((float) image.height()));
	FloatImage image_level = image.get_resize(image_level_width, image_level_height,
						  1, image.spectrum(),
						  parameters.interpolation_type);
	VLOG(1) << "Image Level Size: " << image_level.width() << " x " << image_level.height();
	
	// Truncate the image to fit exactly within the bounds of the bins.
	const int32 overflow_x = image_level.width() % sBins;
	const int32 overflow_y = image_level.height() % sBins;
	if (overflow_x > 0 || overflow_y > 0) {
	  image_level.crop(0, 0, image_level.width() - overflow_x - 1, image_level.height() - overflow_y - 1);
	  VLOG(1) << "Cropping to: " << image_level.width() << " x " << image_level.height();
	}
	
	/***************************************************
	  BEGIN -- Feature Computation for Current Level
	****************************************************/
	FloatImage features;
	if (parameters.featureTypeHOG) {
	  HOGFeatureComputer computer(sBins);
	  features.assign(computer.ComputeFeatures(image_level));
	} else if (parameters.featureTypePatchOnly) {	  
	  FloatImage lab_image_level = 
	    lab_image.get_resize(image_level.width(), image_level.height(), -100, -100, 3);
	  features = lab_image_level.get_channel(0);  // Extract the L channel.
	}
	
	numx = features.width();
	numy = features.height();
	
	if (parameters.useColor) {
	  FloatImage concatenated_features(features.width(), features.height(), 1, features.spectrum() + 2);
	  // First C dimensions are from the already-computed features.
	  cimg_forXYC(features, x, y, c) {
	    concatenated_features(x, y, c) = features(x, y, c);
	  }
	  // The two additional dimensions are the ab channels from
	  // the original image in Lab coordinates resized to a
	  // constant size equal to the (x,y) dimensions of the HOG
	  // features.
	  FloatImage resized_lab_image = lab_image.get_resize(numx, numy, -100, -100, 3);
	  cimg_forXYC(resized_lab_image, x, y, c) {
	    if (c >= 1) {
	      concatenated_features(x, y, features.spectrum() + c - 1) = resized_lab_image(x, y, c);
	    }
	  }
	  features.assign(concatenated_features);
	}
	
	// Compute the gradient of this level's image. For options to this
	// method see:
	// http://cimg.sourceforge.net/reference/structcimg__library_1_1CImg.html#a3e5b54c0b862cbf6e9f14e832984c4d7
	CImgList<float> gradient = image_level.get_gradient("xy", 0);
	if (FLAGS_v >= 3) {
	  image_level.display();
	  (gradient[0], gradient[1]).display();
	}
	// Compute the magnitude of the gradient.
	FloatImage gradient_magnitude(gradient[0].width(), gradient[0].height());
	{
	  FloatImage gradient_magnitude_3 = (gradient[0] * 255.0f).sqr() + (gradient[1] * 255.0f).sqr();
	  cimg_forXY(gradient_magnitude, x, y) {
	    float val = 0.0f;
	    cimg_forC(gradient[0], c) {
	      val += gradient_magnitude_3(x, y, c);
	    }
	    gradient_magnitude(x, y) = val / 3.0f;
	  }
	}
	// Downsample to the correct size.
	gradient_magnitude.resize(numx, numy, -100, -100, 3);  // Bilinear
	if (FLAGS_v >= 3) {
	  gradient_magnitude.display();
	}
	// Save into pyramid.
	pyramid.AddLevel(level, features);
	pyramid.AddGradientLevel(level, gradient_magnitude);
	
	VLOG(1) << "Level " << level << " Size: " << numx << " x " << numy;
      }
      // Set the parameters of the pyramid.
      pyramid.SetCanonicalScale(scale);
      pyramid.SetCanonicalSize(Pair<int32>(numx, numy));
      pyramid.SetScales(scales.get());
      pyramid.SetOriginalImageSize(Pair<int32>(image.width(), image.height()));
      
      return pyramid;
    }
    
    Detector DetectorFactory::LoadFromMatlabFile(const string& filename) {  
      /* open mat file and read it's content */
      MATFile* pmat = matOpen(filename.c_str(), "r");
      if (pmat == NULL) {
	LOG(ERROR) << "Error Opening MAT File: " << filename;
	return Detector();
      }
      
      // Should only have one entry.
      const char* name = NULL;
      mxArray* pa = matGetNextVariable(pmat, &name);

      Detector detector = InitializeFromMatlabArray(*pa);
      mxDestroyArray(pa);
      matClose(pmat);

      return detector;
    }

    Detector DetectorFactory::InitializeFromMatlabArray(const mxArray& array) {
      Detector detector;
      // WARNING: This method assumes that you have a struct containing
      // the relevant fields. The normal output of the pipeline originally
      // was a MATLAB object of typed VisualEntityDetector. There is a
      // file in this directory called ConvertMatlabDetectorToStruct.m
      // that can be used to convert the MATLAB object to a struct. The
      // MATLAB routines for reading in the MATLAB object directly are
      // broken.
      
      // Get the "level" data.
      mxArray* levels = mxGetField(&array, 0, "firstLevModels");
      if (!levels) {
	LOG(ERROR) << "No field \"firstLevModels\" in matrix";
	return detector;
      }
      // Unroll and store in the detector's fields. Assuming these fields
      // are in there and foregoing error checking.
      mxArray* weights_mat = mxGetField(levels, 0, "w");
      mxArray* offsets_mat = mxGetField(levels, 0, "rho");
      mxArray* labels_mat = mxGetField(levels, 0, "firstLabel");
      
      mxArray* infos_mat = mxGetField(levels, 0, "info");
      
      mxArray* thresholds_mat = mxGetField(levels, 0, "threshold");
      
      mxArray* type_mat = mxGetField(levels, 0, "type");
      if (string(mxArrayToString(type_mat)) != "composite") {
	LOG(ERROR) << "Detector type was not composite: " << mxArrayToString(type_mat);
	return detector;
      }
      
      const int32 num_models = mxGetNumberOfElements(offsets_mat);
      const int32 num_weights = mxGetN(weights_mat);

      if (mxIsDouble(weights_mat) && mxIsDouble(offsets_mat) 
	  && mxIsDouble(labels_mat) && mxIsDouble(thresholds_mat)) {
	double* weights_pr = mxGetPr(weights_mat);
	double* offsets_pr = mxGetPr(offsets_mat);
	double* labels_pr = mxGetPr(labels_mat);
	double* thresholds_pr = mxGetPr(thresholds_mat);
	
	for (int i = 0; i < num_models; i++) {
	  vector<float> weights;
	  for (int j = 0; j < num_weights; j++) {
	    // Row-major order.
	    weights.push_back((float) weights_pr[i + j * num_models]);
	  }
	  
	  const float rho = (float) offsets_pr[i];
	  const float first_label = (float) labels_pr[i];
	  const float threshold = (float) thresholds_pr[i];
	  
	  Model model(weights, rho, first_label, threshold);

	  int num_positives = 0;
	  int num_negatives = 0;
	  if (infos_mat != NULL && mxIsCell(infos_mat)) {
	    mxArray* cell = mxGetCell(infos_mat, i);
	    if (cell != NULL && mxIsStruct(cell)) {
	      mxArray* positives_mat = mxGetField(cell, 0, "numPositives");
	      if (positives_mat != NULL && mxIsNumeric(positives_mat)) {
		num_positives = (int) mxGetScalar(positives_mat);
	      }
	      mxArray* negatives_mat = mxGetField(cell, 0, "numNegatives");
	      if (negatives_mat != NULL && mxIsNumeric(negatives_mat)) {
		num_negatives = (int) mxGetScalar(negatives_mat);
	      }
	    }
	  }

	  model.num_positives = num_positives;
	  model.num_negatives = num_negatives;

	  detector.AddModel(model);
	}
      } else if (mxIsSingle(weights_mat) && mxIsSingle(offsets_mat) 
	  && mxIsSingle(labels_mat) && mxIsSingle(thresholds_mat)) {
	float* weights_pr = (float*) mxGetData(weights_mat);
	float* offsets_pr = (float*) mxGetData(offsets_mat);
	float* labels_pr = (float*) mxGetData(labels_mat);
	float* thresholds_pr = (float*) mxGetData(thresholds_mat);
	
	for (int i = 0; i < num_models; i++) {
	  vector<float> weights;
	  for (int j = 0; j < num_weights; j++) {
	    // Row-major order.
	    weights.push_back(weights_pr[i + j * num_models]);
	  }
	  
	  const float rho = offsets_pr[i];
	  const float first_label = labels_pr[i];
	  const float threshold = thresholds_pr[i];
	  
	  Model model(weights, rho, first_label, threshold);
	  detector.AddModel(model);
	}
      } else {
	LOG(ERROR) << "All matrices must be the same type and either float or double.";
	return detector;
      }
      
      // Get the parameters of execution.
      mxArray* parameters = mxGetField(&array, 0, "params");
      if (parameters) {
	detector.SetParameters(LoadParametersFromMatlabMatrix(parameters));
      }
      
      return detector;
    }

  }  // namespace svm
}  // namespace slib
