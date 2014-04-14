#ifndef __SLIB_IMAGE_COLOR_HISTOGRAM_FEATURE_COMPUTER_H__
#define __SLIB_IMAGE_COLOR_HISTOGRAM_FEATURE_COMPUTER_H__

#include "feature_computer.h"

#include <CImg.h>
#include <common/types.h>
#include <gflags/gflags.h>

DECLARE_int32(color_histogram_feature_computer_color_bins);

namespace slib {
  namespace image {

    class ColorHistogramFeatureComputer : public FeatureComputer {
    public:
      // @parameter spatial_bins - The size of a 'patch' when
      // computing the histogram. Each spatial_bins x spatial_bins
      // patch in the image will be reduced to one histogram.
      //
      // @parameter color_bins - The number of bins that the colors are binned to.
      ColorHistogramFeatureComputer(const int& spatial_bins);

      virtual FloatImage ComputeFeatures(const FloatImage& image) const;
      virtual FeaturePyramid ComputeFeaturePyramid(const FloatImage& image, 
						   const float& image_canonical_size,
						   const int32& scale_intervals = 8, 
						   const Pair<int32>& patch_size = Pair<int32>(80, 80),
						   const std::vector<int32>& levels = std::vector<int32>(0)) const;

      static int GetPatchChannels();

    private:
      int _spatial_bins;
    };

  }
}

#endif  // __SLIB_IMAGE_COLOR_HISTOGRAM_FEATURE_COMPUTER_H__
