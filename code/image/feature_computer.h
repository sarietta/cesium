#ifndef __SLIB_IMAGE_FEATURE_COMPUTER_H__
#define __SLIB_IMAGE_FEATURE_COMPUTER_H__

#define SLIB_NO_DEFINE_64BIT

#include <CImg.h>
#include "../common/types.h"

namespace slib {
  namespace image {

    class FeatureComputer {
    public:
      static FloatImage ComputeHOGFeatures(const FloatImage& image, const int32& bins);
    };
  }  // namespace image
}  // namespace slib

#endif
