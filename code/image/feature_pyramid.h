#ifndef __SLIB_IMAGE_FEATURE_PYRAMID_H__
#define __SLIB_IMAGE_FEATURE_PYRAMID_H__

#define SLIB_NO_DEFINE_64BIT

#include <CImg.h>
#include "../common/scoped_ptr.h"
#include "../common/types.h"
#include <float.h>
#undef Success
#include <Eigen/Dense>
#include <math.h>
#include "../util/assert.h"
#include <vector>

namespace slib {
  namespace image {

    class FeaturePyramid {
    public:
      FeaturePyramid(const int& num_levels);
      FeaturePyramid(const FeaturePyramid& pyramid);

      // Throws away features (and corresponding indices and level
      // information) whose gradient sums are less than the specified
      // threshold.
      static FloatMatrix ThresholdFeatures(const FloatMatrix& all_features,
					   const std::vector<float>& gradient_sums,
					   const float& threshold,					   
					   std::vector<int32>* levels,
					   std::vector<Pair<int32> >* indices);

      // Gets the actual feature vector for the specified level. The
      // output of this method is a matrix of size (# of features) x
      // (feature_dimensions) That is, there are rows = the number of
      // features and cols = feature dimensions. 
      FloatMatrix GetLevelFeatureVector(const int& index, const Pair<int32>& patch_size, 
					const int32& feature_dimensions,
					std::vector<int32>* levels = NULL,
					std::vector<Pair<int32> >* indices = NULL,
                                        std::vector<float>* gradient_sums = NULL) const;
      // Does the same thing as above, but stores the results in the
      // features pointer.
      void GetLevelFeatureVector(const int& index, 
				 const Pair<int32>& patch_size, 
				 const int32& feature_dimensions,
				 float* features,
				 std::vector<int32>* levels = NULL,
				 std::vector<Pair<int32> >* indices = NULL,
                                 std::vector<float>* gradient_sums = NULL) const;

      // A relatively fast way to get all of the features in one
      // matrix. There are cols = feature_dimensions and rows = total
      // # of features.
      FloatMatrix GetAllLevelFeatureVectors(const Pair<int32>& patch_size, 
					    const int32& feature_dimensions,
					    std::vector<int32>* levels = NULL,
					    std::vector<Pair<int32> >* indices = NULL,
                                            std::vector<float>* gradient_sums = NULL) const;

      void AddLevel(const int& index, const FloatImage& level);
      void AddGradientLevel(const int& index, const FloatImage& gradient);
      void SetCanonicalScale(const float& scale);
      void SetCanonicalSize(const Pair<int32>& size);
      void SetScales(const float* scales);
      void SetScales(const std::vector<float>& scales);
      void SetOriginalImageSize(const Pair<int32>& size);

      bool SaveToFile(const std::string& filename) const;
      
      inline int32 GetNumLevels() const {
	return _num_levels;
      }

      inline FloatImage& GetLevel(const int& index) const {
	return _levels[index];
      }

      inline FloatImage& GetGradientLevel(const int& index) const {
	return _gradient_levels[index];
      }

      inline float GetCanonicalScale() const {
	return _canonical_scale;
      }

      inline Pair<int32> GetCanonicalSize() const {
	return _canonical_size;
      }

      inline std::vector<float> GetScales() const {
	return _scales;
      }

      inline Pair<int32> GetOriginalImageSize() const {
	return _original_image_size;
      }

      static inline Pair<Pair<float> > GetPatchSizeInLevel(const Pair<int32>& patch_size, 
							   const float& level_scale,
							   const float& canonical_scale,
							   const int32 bins) {
	const float x2 = 
	  round((((float) patch_size.x) + 2.0f) * ((float) bins) * level_scale / canonical_scale) 
	  - 1.0f;
	const float y2 = 
	  round((((float) patch_size.y) + 2.0f) * ((float) bins) * level_scale / canonical_scale) 
	  - 1.0f;
      
	return Pair<Pair<float> >(Pair<float>(0.0f, 0.0f), Pair<float>(x2, y2));
      }

      inline Pair<Pair<float> > GetPatchSizeInLevel(const Pair<int32>& patch_size, const int32& level,
						    const int32 bins) const {
	ASSERT_LT((uint32) level, _scales.size());
	const float level_scale = _scales[level];
	const float canonical_scale = _canonical_scale;

	return FeaturePyramid::GetPatchSizeInLevel(patch_size, level_scale, canonical_scale, bins);
      }

    private:
      int _num_levels;
      scoped_array<FloatImage> _levels;
      scoped_array<FloatImage> _gradient_levels;

      float _canonical_scale;
      Pair<int32> _canonical_size;
      std::vector<float> _scales;
      Pair<int32> _original_image_size;
    };

  }  // namespace image
}  // namespace slib

#endif
