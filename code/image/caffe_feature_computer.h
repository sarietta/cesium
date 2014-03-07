#ifndef __SLIB_IMAGE_CAFFE_FEATURE_COMPUTER_H__
#define __SLIB_IMAGE_CAFFE_FEATURE_COMPUTER_H__

#ifndef SKIP_CAFFE_FEATURE_COMPUTER

#define SLIB_NO_DEFINE_64BIT

#include "feature_computer.h"

#include <CImg.h>
#include <common/types.h>

namespace caffe {
  template <typename T> class Blob;
  template <typename T> class Net;
}

namespace FFLD {
  class JPEGImage;
  class Patchwork;
}

namespace slib {
  namespace image {
    class FeaturePyramid;
  }
}

namespace slib {
  namespace image {

    class CaffeFeatureComputer : public FeatureComputer {
    public:
      CaffeFeatureComputer(const std::string& network_proto_filename, const std::string& network_filename);
      virtual FloatImage ComputeFeatures(const FloatImage& image) const;

      virtual FeaturePyramid ComputeFeaturePyramid(const FloatImage& image, 
						   const float& image_canonical_size,
						   const int32& scale_intervals = 8, 
						   const Pair<int32>& patch_size = Pair<int32>(80, 80),
						   const std::vector<int32>& levels = std::vector<int32>(0)) const;

    private:
      // The protocol buffer that describes the network.
      std::string _network_proto_filename;
      // The protocol buffer that contains the trained network.
      std::string _network_filename;

      // These next three functions are borrowed from the Caffe code,
      // but are tailored to suit the needs of the FeatureComputer.
      int GetBinsForNet(caffe::Net<float>& net) const;
      FFLD::Patchwork CreatePatchwork(const FloatImage& cimage, const int& img_minWidth, 
				      const int& img_minHeight, const int& padding, 
				      const int& interval, const int& planeDim) const;
      void PlaneToBlobProto(const FFLD::JPEGImage& plane, caffe::Blob<float>* blob) const;

    };
  }  // namespace image
}  // namespace slib

#endif  // #ifndef SKIP_CAFFE_FEATURE_COMPUTER

#endif
