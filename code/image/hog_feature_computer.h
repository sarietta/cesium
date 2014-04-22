#ifndef __SLIB_IMAGE_HOG_FEATURE_COMPUTER_H__
#define __SLIB_IMAGE_HOG_FEATURE_COMPUTER_H__

#define SLIB_NO_DEFINE_64BIT

#include "feature_computer.h"

#include <CImg.h>
#include <common/types.h>

namespace slib {
  namespace image {

    class HOGFeatureComputer : public FeatureComputer {
    public:
      explicit HOGFeatureComputer(const int32& sBins);
      virtual FloatImage ComputeFeatures(const FloatImage& image) const;

      virtual Pair<float> GetPatchSize(const Pair<float>& canonical_patch_size) const;
      static Pair<float> GetPatchSize(const Pair<float>& canonical_patch_size, const float& sbins);

      static int GetPatchChannels();

    private:
      int32 _sBins;

      FloatImage ComputeHOGFeatures(const FloatImage& image, const int32& bins) const;
    };
  }  // namespace image
}  // namespace slib

#endif
