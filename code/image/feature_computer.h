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

      // This function must return the effective size of a patch given
      // the size of a patch in an image of "canonical
      // size". Sometimes this may just return the same patch size
      // (the default implementation), but often FeatureComputers have
      // a different output width/height than the input images. This
      // function effectively dictates that scaling, although it can
      // include other potential effects like boundary isssues so it
      // should NOT be interpreted as a scale directly.
      //
      // Typically it's preferred that you can create a static method
      // in the base class that takes the relevant parameters (stride,
      // etc) and then have this member function call the static
      // method. This makes it easier for external programs and
      // functions to determine this information easily.
      virtual Pair<float> GetPatchSize(const Pair<float>& canonical_patch_size) const;

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
