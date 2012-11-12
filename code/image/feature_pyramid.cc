#include "feature_pyramid.h"

#include "../util/statistics.h"
#include <glog/logging.h>
#include <string>
#include <string.h>
#include <vector>

using std::string;
using std::vector;

namespace slib {
  namespace image {

    FeaturePyramid::FeaturePyramid(const FeaturePyramid& pyramid) {
      _num_levels = pyramid.GetNumLevels();

      _levels.reset(new FloatImage[_num_levels]);
      for (int i = 0; i < _num_levels; i++) {
	_levels[i].assign(pyramid.GetLevel(i));
      }
      _gradient_levels.reset(new FloatImage[_num_levels]);
      for (int i = 0; i < _num_levels; i++) {
	_gradient_levels[i].assign(pyramid.GetGradientLevel(i));
      }

      SetCanonicalScale(pyramid.GetCanonicalScale());
      SetCanonicalSize(pyramid.GetCanonicalSize());
      SetScales(pyramid.GetScales());
      SetOriginalImageSize(pyramid.GetOriginalImageSize());
    }

    FeaturePyramid::FeaturePyramid(const int& num_levels) 
      : _num_levels(num_levels) {
      _levels.reset(new FloatImage[_num_levels]);
      _gradient_levels.reset(new FloatImage[_num_levels]);
    }

    FloatMatrix FeaturePyramid::ThresholdFeatures(const FloatMatrix& all_features,
						  const vector<float>& gradient_sums,
						  const float& threshold,					   
						  vector<int32>* levels,
						  vector<Pair<int32> >* indices) {
      vector<int32> invalid;
      for (uint32 i = 0; i < gradient_sums.size(); i++) {
	if (gradient_sums[i] < threshold) {
	  invalid.push_back(i);
	}
      }
      LOG(INFO) << "Found " << invalid.size() << " invalid patches";
	
      if (invalid.size() == 0) {
	return all_features;
      }

      FloatMatrix features(all_features.rows() - invalid.size(), all_features.cols());
      
      int invalid_index = 0;
      for (int i = 0; i < all_features.rows(); i++) {
	if (i == invalid[invalid_index]) {  // Because they are pre-sorted.
	  levels->erase(levels->begin() + i - invalid_index);
	  indices->erase(indices->begin() + i - invalid_index);
	  invalid_index++;
	} else {
	  features.row(i - invalid_index) = all_features.row(i);
	}
      }

      return features;
    }

    FloatMatrix FeaturePyramid::GetLevelFeatureVector(const int& index, 
						      const Pair<int32>& patch_size, 
						      const int32& feature_dimensions,
						      vector<int32>* levels,
						      vector<Pair<int32> >* indices,
                                                      vector<float>* gradient_sums) const {
      const int32 rLim = _levels[index].height() - patch_size.x + 1;
      const int32 cLim = _levels[index].width() - patch_size.y + 1;

      FloatMatrix features(rLim * cLim, feature_dimensions);
      GetLevelFeatureVector(index, patch_size, feature_dimensions, &features(0), 
			    levels, indices, gradient_sums);
      return features;
    }

    void FeaturePyramid::GetLevelFeatureVector(const int& index, 
					       const Pair<int32>& patch_size, 
					       const int32& feature_dimensions,
					       float* features,
					       vector<int32>* levels,
					       vector<Pair<int32> >* indices,
                                               vector<float>* gradient_sums) const {
      const FloatImage& level = _levels[index];
      const int32 rLim = level.height() - patch_size.x + 1;
      const int32 cLim = level.width() - patch_size.y + 1;

      int num_features = 0;
      for (int j = 0; j < cLim; j++) {
	for (int i = 0; i < rLim; i++) {
	  if (gradient_sums) {
	    // Unrolled patch of gradient values.
	    const FloatImage& gradient 
	      = _gradient_levels[index].get_crop(j, i, j + patch_size.y - 1, i + patch_size.x - 1);
	    const float* gradient_data = gradient.data();
	    gradient_sums->push_back(slib::util::Mean<float>(gradient_data, patch_size.x * patch_size.y));
	  }
	  // Unrolled patch of "features" values (HOG, color, etc).
	  const FloatImage feature = level
	    .get_crop(j, i, j + patch_size.y - 1, i + patch_size.x - 1).transpose().unroll('x');
	  const float* feature_data = feature.data();
	  // Fast copy.
	  memcpy(features + feature_dimensions * num_features, feature_data, sizeof(float) * feature_dimensions);
	  if (indices) {
	    indices->push_back(Pair<int32>(j, i));
	  }
	  if (levels) {
	    levels->push_back(index);
	  }

	  num_features++;
	}
      }
    }

    FloatMatrix FeaturePyramid::GetAllLevelFeatureVectors(const Pair<int32>& patch_size, 
							  const int32& feature_dimensions,
							  vector<int32>* levels,
							  vector<Pair<int32> >* indices,
                                                          vector<float>* gradient_sums) const {
      int32 total_features = 0;
      for (int i = 0; i < GetNumLevels(); i++) {
	const int32 rLim = _levels[i].height() - patch_size.x + 1;
	const int32 cLim = _levels[i].width() - patch_size.y + 1;
	total_features += (rLim * cLim);
      }

      FloatMatrix features(total_features, feature_dimensions);
      float* features_data = features.data();
      for (int i = 0; i < GetNumLevels(); i++) {
	GetLevelFeatureVector(i, patch_size, feature_dimensions, features_data,
			      levels, indices, gradient_sums);
	const int32 rLim = _levels[i].height() - patch_size.x + 1;
	const int32 cLim = _levels[i].width() - patch_size.y + 1;
	features_data += (rLim * cLim * feature_dimensions);
      }

      return features;
    }

    void FeaturePyramid::AddLevel(const int& index, const FloatImage& level) {
      _levels[index].assign(level);
    }
     
    void FeaturePyramid::AddGradientLevel(const int& index, const FloatImage& gradient) {
      _gradient_levels[index].assign(gradient);
    }

    void FeaturePyramid::SetOriginalImageSize(const Pair<int32>& size) {
      _original_image_size = size;
    }

    void FeaturePyramid::SetCanonicalScale(const float& scale) {
      _canonical_scale = scale;
    }

    void FeaturePyramid::SetCanonicalSize(const Pair<int32>& size) {
      _canonical_size = size;
    }

    void FeaturePyramid::SetScales(const float* scales) {
      for (int i = 0; i < _num_levels; i++) {
	_scales.push_back(scales[i]);
      }
    }

    void FeaturePyramid::SetScales(const std::vector<float>& scales) {
      for (uint32 i = 0; i < scales.size(); i++) { 
	_scales.push_back(scales[i]);
      }
    }

    bool FeaturePyramid::SaveToFile(const string& filename) const {
      FILE* fid = fopen(filename.c_str(), "wb");
      if (!fid) { 
	LOG(ERROR) << "Could not open file for writing: " << filename;
	return false;
      }

      int count = 0;
      count += fwrite(&_num_levels, sizeof(int), 1, fid);
      count += fwrite(&_canonical_scale, sizeof(float), 1, fid);
      count += fwrite(&_canonical_size.x, sizeof(int32), 1, fid);
      count += fwrite(&_canonical_size.y, sizeof(int32), 1, fid);
      count += fwrite(&_original_image_size.x, sizeof(int32), 1, fid);
      count += fwrite(&_original_image_size.y, sizeof(int32), 1, fid);

      if (count != 6) {
	LOG(ERROR) << "Could not save pyramid to file: " + filename;
	fclose(fid);
	return false;
      }

      count = 0;

      const int num_scales = (int) _scales.size();
      count += fwrite(&num_scales, sizeof(int), 1, fid);
      for (uint32 i = 0; i < _scales.size(); i++) {
	count += fwrite(&_scales[i], sizeof(float), 1, fid);
      }
      if ((uint32) count != 1 + _scales.size()) {
	LOG(ERROR) << "Could not save pyramid to file: " + filename;
	fclose(fid);
	return false;
      }

      // Feature levels.
      for (int i = 0; i < _num_levels; i++) {
	const FloatImage& level = _levels[i];
	const int width = level.width();
	const int height = level.height();
	const int depth = level.depth();
	const int channels = level.spectrum();

	count = 0;
	count += fwrite(&width, sizeof(int), 1, fid);
	count += fwrite(&height, sizeof(int), 1, fid);
	count += fwrite(&depth, sizeof(int), 1, fid);
	count += fwrite(&channels, sizeof(int), 1, fid);
	count += fwrite(level.data(), sizeof(float), width * height * depth * channels, fid);

	if (count != 4 + width * height * depth * channels) {
	  LOG(ERROR) << "Could not save pyramid to file: " + filename;
	  fclose(fid);
	  return false;
	}
      }

      // Gradient levels.
      for (int i = 0; i < _num_levels; i++) {
	const FloatImage& level = _gradient_levels[i];
	const int width = level.width();
	const int height = level.height();
	const int depth = level.depth();
	const int channels = level.spectrum();

	count = 0;
	count += fwrite(&width, sizeof(int), 1, fid);
	count += fwrite(&height, sizeof(int), 1, fid);
	count += fwrite(&depth, sizeof(int), 1, fid);
	count += fwrite(&channels, sizeof(int), 1, fid);
	count += fwrite(level.data(), sizeof(float), width * height * depth * channels, fid);

	if (count != 4 + width * height * depth * channels) {
	  LOG(ERROR) << "Could not save pyramid to file: " + filename;
	  fclose(fid);
	  return false;
	}
      }

      fclose(fid);
      return true;
    }
  }  // namespace image
}  // namespace slib
