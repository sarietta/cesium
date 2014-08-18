#ifndef __SLIB_IMAGE_CAFFE_FEATURE_COMPUTER_H__
#define __SLIB_IMAGE_CAFFE_FEATURE_COMPUTER_H__

#ifndef SKIP_CAFFE_FEATURE_COMPUTER

#define SLIB_NO_DEFINE_64BIT

#include "feature_computer.h"

#include <caffe/net.hpp>  // To avoid incomplete type in files that include this file.
#include <CImg.h>
#include <common/scoped_ptr.h>
#include <common/types.h>
#include <gflags/gflags.h>

DECLARE_string(caffe_network_description_filename);
DECLARE_string(caffe_trained_network_filename);
DECLARE_int32(caffe_feature_computer_upsample_factor);
DECLARE_int32(caffe_feature_computer_padding);

namespace caffe {
  template <typename T> class Blob;
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
      virtual Pair<float> GetPatchSize(const Pair<float>& canonical_patch_size) const;
      static Pair<float> GetPatchSize(const Pair<float>& canonical_patch_size, const float& bins);

      virtual FloatImage ComputeFeatures(const FloatImage& image) const;

      virtual FeaturePyramid ComputeFeaturePyramid(const FloatImage& image, 
						   const float& image_canonical_size,
						   const int32& scale_intervals = 8, 
						   const Pair<int32>& patch_size = Pair<int32>(80, 80),
						   const std::vector<int32>& levels = std::vector<int32>(0)) const;

      static bool Finalize();

      static int GetBinsForNet(const caffe::Net<float>& net);
      static int GetPatchChannels();
      inline const caffe::Net<float>& GetNet() const {
	return *(_caffe.get());
      }

      inline caffe::Net<float>* GetMutableNet() {
	return _caffe.get();
      }

      static CaffeFeatureComputer* GetInstance();      

    private:
      scoped_ptr<caffe::Net<float> > _caffe;

      static scoped_ptr<CaffeFeatureComputer> _instance;

      CaffeFeatureComputer();
      CaffeFeatureComputer(const std::string& network_description_filename, 
			   const std::string& trained_network_filename);

      void LoadModel(const std::string& network_description_filename, 
		     const std::string& trained_network_filename);

      // These next three functions are borrowed from the Caffe code,
      // but are tailored to suit the needs of the FeatureComputer.
      FFLD::Patchwork CreatePatchwork(const FloatImage& cimage, const int& img_minWidth, 
				      const int& img_minHeight, const int& padding, 
				      const int& interval, const int& planeDim) const;
      void PlaneToBlobProto(const FFLD::JPEGImage& plane, caffe::Blob<float>* blob) const;

    };
  }  // namespace image
}  // namespace slib

#endif  // #ifndef SKIP_CAFFE_FEATURE_COMPUTER

#endif
