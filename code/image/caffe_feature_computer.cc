#include "caffe_feature_computer.h"

#ifndef SKIP_CAFFE_FEATURE_COMPUTER

#undef Status
#include <caffe/caffe.hpp>
#include <caffe/featpyra_common.hpp>
#include <caffe/Patchwork.h>
#include <caffe/PyramidStitcher.h>
#include <common/scoped_ptr.h>
#include <cstring>
#include <cstdlib>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <image/feature_pyramid.h>
#include <string/stringutils.h>
#include <util/assert.h>
#include <util/matlab.h>
#include <vector>

DEFINE_string(caffe_network_description_filename, "", 
	      "The file (protobuffer) that contains the description of the trained network specified below.");
DEFINE_string(caffe_trained_network_filename, "",
	      "The file that contains a trained caffe model.");
DEFINE_int32(caffe_feature_computer_upsample_factor, 1, "Amount to upsample images by before computing features.");
DEFINE_int32(caffe_feature_computer_padding, 1, "The amount of padding to add to the image.");

using caffe::Blob;
using caffe::Caffe;
using caffe::Net;
using caffe::NetParameter;
using FFLD::JPEGImage;
using FFLD::JPEGPyramid;
using FFLD::Patchwork;
using slib::StringUtils;
using slib::image::FeaturePyramid;
using slib::util::MatlabMatrix;
using std::vector;

namespace slib {
  namespace image {

    scoped_ptr<CaffeFeatureComputer> CaffeFeatureComputer::_instance(NULL);
    
    CaffeFeatureComputer* CaffeFeatureComputer::GetInstance() {
      if (_instance.get() == NULL) {
	_instance.reset(new CaffeFeatureComputer(FLAGS_caffe_network_description_filename, 
						 FLAGS_caffe_trained_network_filename));
      }
      return _instance.get();
    }

    CaffeFeatureComputer::CaffeFeatureComputer() {
      LoadModel(FLAGS_caffe_network_description_filename, FLAGS_caffe_trained_network_filename);
    }

    CaffeFeatureComputer::CaffeFeatureComputer(const string& network_description_filename, 
					       const string& trained_network_filename) {
      LoadModel(network_description_filename, trained_network_filename);
    }

    void CaffeFeatureComputer::LoadModel(const string& network_description_filename, 
					 const string& trained_network_filename) {
      VLOG(1) << "Setting up initial parameters for Caffe";
      Caffe::set_phase(Caffe::TEST);
      Caffe::set_mode(Caffe::CPU);

      VLOG(1) << "Loading network definition proto from file: " << network_description_filename;
      NetParameter test_net_param;
      ReadProtoFromTextFile(network_description_filename, &test_net_param);
      
      VLOG(1) << "Initializing Caffe";
      _caffe.reset(new Net<float>(test_net_param));

      VLOG(1) << "Loading pretrained network from file: " << trained_network_filename;
      NetParameter trained_net_param;
      ReadProtoFromBinaryFile(trained_network_filename, &trained_net_param);
      _caffe->CopyTrainedLayersFrom(trained_net_param);
    }
    
    Pair<float> CaffeFeatureComputer::GetPatchSize(const Pair<float>& canonical_patch_size) const {
      return CaffeFeatureComputer::GetPatchSize(canonical_patch_size, GetBinsForNet(*_caffe.get()));
    }

    Pair<float> CaffeFeatureComputer::GetPatchSize(const Pair<float>& canonical_patch_size, const float& bins) {
      Pair<float> patch_size;

      patch_size.x = round(canonical_patch_size.x / bins) - 2.0f;
      patch_size.y = round(canonical_patch_size.y / bins) - 2.0f;
      
      return patch_size;
    }

    int CaffeFeatureComputer::GetBinsForNet(const Net<float>& net) {
      const vector<uint32_t>& strides = net.layer_strides();
      int bins = 1;
      for (vector<uint32_t>::const_iterator i = strides.begin(); i != strides.end(); ++i) { 
	if (*i) { 
	  bins *= (*i); 
	} 
      }
      return bins;
    }

    int CaffeFeatureComputer::GetPatchChannels() {
      // Shouldn't actually be mutable, but Caffe class doesn't have
      // CV qualifiers for const access to this information.
      return GetInstance()->GetMutableNet()->output_blobs()[0]->channels();
    }
    
    Patchwork CaffeFeatureComputer::CreatePatchwork(const FloatImage& cimage, const int& img_minWidth, 
						    const int& img_minHeight, const int& padding, 
						    const int& interval, const int& planeDim) const {
      scoped_array<uint8_t> bits(new uint8_t[cimage.width() * cimage.height() * cimage.spectrum()]);
      cimg_forXYC(cimage, x, y, c) {
	bits[c + cimage.spectrum() * (x + cimage.width() * y)] = static_cast<uint8_t>(255.0f * cimage(x, y, c));
      }
      JPEGImage image(cimage.width(), cimage.height(), cimage.spectrum(), bits.get());
      
      const int upsampleFactor = FLAGS_caffe_feature_computer_upsample_factor;
      
      // Compute the downsample+stitch
      // 	multiscale DOWNSAMPLE with (padx == pady == padding)
      JPEGPyramid pyramid(image, padding, padding, interval, upsampleFactor); 
      if (pyramid.empty()) {
	LOG(ERROR) << "Could not construct pyramid.";
      }
      
      int planeWidth; 
      int planeHeight;
      
      if(planeDim > 0){
	planeWidth = planeDim;
	planeHeight = planeDim;
      } 
      else{
	planeWidth = (pyramid.levels()[0].width() + 15) & ~15; 
	planeHeight = (pyramid.levels()[0].height() + 15) & ~15; 
	planeWidth = max(planeWidth, planeHeight);  //SQUARE planes for Caffe convnet
	planeHeight = max(planeWidth, planeHeight);
      }
      
      Patchwork::Init(planeHeight, planeWidth, img_minWidth, img_minHeight); 
      const Patchwork patchwork(pyramid); //STITCH
      
      return patchwork;
    }
    
    
    void CaffeFeatureComputer::PlaneToBlobProto(const JPEGImage& plane, Blob<float>* blob) const {
      const int depth = plane.depth();
      const int height = plane.height();
      const int width = plane.width();
      
      scoped_array<float> bits(new float[depth * width * height]);
      for (int ch_src = 0; ch_src < depth; ch_src++) { //ch_src is in RGB
	const int ch_dst = caffe::get_BGR(ch_src); //for Caffe BGR convention
	const float ch_mean = IMAGENET_MEAN_RGB[ch_src];
	
	for (int y = 0; y < height; y++) {
	  for (int x = 0; x < width; x++) {
	    bits[ch_src * width * height + y * width + x] 
	      = plane.bits()[y * width * depth + x * depth + ch_src] - ch_mean;
	  }
	}
      }
      
      blob->Reshape(1, depth, height, width);
      memcpy(blob->mutable_cpu_data(), bits.get(), sizeof(float) * width * height * depth);
      blob->Update();
    }
        
    FloatImage CaffeFeatureComputer::ComputeFeatures(const FloatImage& image) const {
      LOG(ERROR) << "Do not call CaffeFeatureComputer::ComputeFeatures directly. "
		 << "Use the ComputeFeaturePyramid(string, ...) instead.";
      return FloatImage();
    }
    
    FeaturePyramid CaffeFeatureComputer::ComputeFeaturePyramid(const FloatImage& original_image,
							       const float& image_canonical_size,
							       const int32& scale_intervals, 
							       const Pair<int32>& patch_size,
							       const vector<int32>& levels) const {
      // Image should have pixels in [0,1]
      if (original_image.max() > 1.0f) {
	LOG(ERROR) << "Image pixels must lie in the domain [0,1] (Max: " << original_image.max() << ")";
	return FeaturePyramid(0);
      }

      // Scale the image to the canonical size.
      float scale = 0.0f;
      if (original_image.width() < original_image.height()) {
	scale = image_canonical_size / static_cast<float>(original_image.width());
      } else {
	scale = image_canonical_size / static_cast<float>(original_image.height());
      }

      if (image_canonical_size <= 0) {
	scale = 1.0f;
      }

      VLOG(1) << "Canonical Scale: " << scale;
      
      const float scaled_width = scale * original_image.width();
      const float scaled_height = scale * original_image.height();

      const Pair<int32> original_image_size(original_image.width(), original_image.height());      
      const FloatImage image = original_image.get_resize(scaled_width, scaled_height, -100, -100, 5);
            
      const int bins = GetBinsForNet(*_caffe.get());
      const int resultDepth = _caffe->output_blobs()[0]->channels();
      const int planeDim = _caffe->input_blobs()[0]->width();
            
      //const int padding = patch_size.x / 2;
      const int padding = FLAGS_caffe_feature_computer_padding;

      const Pair<int> min_level_size = CaffeFeatureComputer::GetPatchSize(patch_size, bins);

      VLOG(1) << "Creating patchwork from image";
      VLOG(1) << "Min Level Size: " << min_level_size.x << "x" << min_level_size.y;
      Patchwork patchwork = CreatePatchwork(image, min_level_size.x, min_level_size.y, 
					    padding, scale_intervals, planeDim); 
      int nbPlanes = patchwork.planes_.size();
      
      scoped_array<Blob<float> > output_blobs(new Blob<float>[nbPlanes]);
      VLOG(1) << "Propagating " << nbPlanes << " patchwork planes through network";
      for (int planeID = 0; planeID < nbPlanes; planeID++) {
	const JPEGImage& currPlane = patchwork.planes_[planeID];
	
	VLOG(1) << "Level " << planeID << " Size: (" 
		<< currPlane.width() << ", " 
		<< currPlane.height() << ", " 
		<< currPlane.depth() << ")";
	
	vector<Blob<float>*>& input_blobs = _caffe->input_blobs();
	ASSERT_EQ(input_blobs.size(), 1);
	PlaneToBlobProto(currPlane, input_blobs[0]);

	const vector<Blob<float>*>& output = _caffe->ForwardPrefilled();
	ASSERT_EQ(output.size(), 1);
	output_blobs[planeID].CopyFrom(*output[0], false, true);
	output_blobs[planeID].Update();
      }  
      
      vector<ScaleLocation> scaleLocations = unstitch_pyramid_locations(patchwork, bins);
      const int nbScales = scaleLocations.size();
      
      FeaturePyramid pyramid(nbScales);
      const float sc = pow(2.0f, 1.0f / ((float) scale_intervals));
      vector<float> scales(nbScales);
#if 0
      for (int i = 0; i < nbScales; i++) {
	scales[i] = pow(sc, (float) i);
	VLOG(2) << "Scale for level " << i << ": " << scales[i];
      }
#endif     
 
      VLOG(1) << "Copying features into output FeaturePyramid";
      for (int i = 0; i < nbScales; i++) {
	const int planeID = scaleLocations[i].planeID;
	ASSERT_LT(planeID, (int) nbPlanes);
	
	Blob<float>& level_blob = output_blobs[planeID];
	const int xmin = scaleLocations[i].xMin;
	const int ymin = scaleLocations[i].yMin;
	
	const int width = scaleLocations[i].width;
	const int height = scaleLocations[i].height;
	const int channels = resultDepth;
#if 1
	scales[i] = ((float) scaleLocations[0].width) / ((float) (width));
#endif
	
	const int blob_width = level_blob.width();
	
	VLOG(1) << "Found blob at level: " << (i+1) << " [" << planeID << "]"
		<< " (" << width << ", " << height << ", " << channels << ")";
	
	const float* bits = level_blob.cpu_data();
	FloatImage full_level(level_blob.width(), level_blob.height(), level_blob.channels());
	memcpy(full_level.data(), bits, sizeof(float) * level_blob.count());
	
	FloatImage level(width, height, channels);
	cimg_forXYZ(level, x, y, c) {
	  const int levelx = x + xmin;
	  const int levely = y + ymin;
	  level(x, y, c) = full_level(levelx, levely, c);
	}

	const float level_scale = scale / scales[i];
	VLOG(1) << "Scale: " << scales[i];
	VLOG(1) << "Level Scale: " << level_scale;

	const float image_level_width = ceil(level_scale * ((float) image.width()));
	const float image_level_height = ceil(level_scale * ((float) image.height()));
	const FloatImage image_level = image.get_resize(image_level_width, image_level_height,
							1, image.spectrum(), 5);	

	const FloatImage gradient_magnitude = ComputeGradientMagnitude(image_level, 
								       image_level_width, 
								       image_level_height);
	
	pyramid.AddLevel(i, level);
	pyramid.AddGradientLevel(i, gradient_magnitude);
      }
      
      pyramid.SetCanonicalScale(scale);
      pyramid.SetCanonicalSize(Pair<int32>(0, 0));
      pyramid.SetScales(scales);
      pyramid.SetOriginalImageSize(original_image_size);
      
      return pyramid;
    }
  }  // namespace image
}  // namespace slib


#endif  // #ifndef SKIP_CAFFE_FEATURE_COMPUTER
