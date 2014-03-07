#ifndef __SLIB_IMAGE_FEATURE_COMPUTER_H__
#define __SLIB_IMAGE_FEATURE_COMPUTER_H__

#define SLIB_NO_DEFINE_64BIT

#include <CImg.h>
#include <common/types.h>
#include <vector>

namespace slib {
  namespace image {
    class FeaturePyramid;
  }
}

namespace slib {
  namespace image {

    class FeatureComputer {
    public:
      virtual FloatImage ComputeFeatures(const FloatImage& image) const = 0;
      virtual FeaturePyramid ComputeFeaturePyramid(const FloatImage& image, 
						   const float& image_canonical_size,
						   const int32& scale_intervals = 8, 
						   const Pair<int32>& patch_size = Pair<int32>(80, 80),
						   const std::vector<int32>& levels = std::vector<int32>(0)) const;

      virtual FloatImage ComputeFeatures(const std::string& image_filename) const;
      virtual FeaturePyramid ComputeFeaturePyramid(const std::string& image_filename, 
						   const float& image_canonical_size,
						   const int32& scale_intervals = 8, 
						   const Pair<int32>& patch_size = Pair<int32>(80, 80),
						   const std::vector<int32>& levels = std::vector<int32>(0)) const;

    protected:
      FloatImage ComputeGradientMagnitude(const FloatImage& image, 
					  const int& output_width, const int& output_height) const;
    };
  }  // namespace image
}  // namespace slib

#endif
