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

using caffe::Blob;
using caffe::Caffe;
using caffe::densenet_params_t;
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
    
    int CaffeFeatureComputer::GetBinsForNet(Net<float>& net) const {
      const vector<uint32_t>& strides = net.layer_strides();
      int bins = 1;
      for (vector<uint32_t>::const_iterator i = strides.begin(); i != strides.end(); ++i) { 
	if (*i) { 
	  bins *= (*i); 
	} 
      }
      return bins;
    }
    
    Patchwork CaffeFeatureComputer::CreatePatchwork(const FloatImage& cimage, const int& img_minWidth, 
						    const int& img_minHeight, const int& padding, 
						    const int& interval, const int& planeDim) const {
      scoped_array<uint8_t> bits(new uint8_t[cimage.width() * cimage.height() * cimage.spectrum()]);
      cimg_forXYC(cimage, x, y, c) {
	bits[c + cimage.spectrum() * (x + cimage.width() * y)] = static_cast<uint8_t>(255.0f * cimage(x, y, c));
      }
      JPEGImage image(cimage.width(), cimage.height(), cimage.spectrum(), bits.get());
      
      int upsampleFactor = 2; //TODO: make this an input param?
      
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
    
    CaffeFeatureComputer::CaffeFeatureComputer(const string& network_proto_filename, 
					       const string& network_filename) 
      : _network_proto_filename(network_proto_filename), _network_filename(network_filename) {
      VLOG(1) << "Setting up initial parameters for Caffe";
      Caffe::set_phase(Caffe::TEST);
      Caffe::set_mode(Caffe::CPU);
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
      
      VLOG(1) << "Loading network definition proto from file: " << _network_proto_filename;
      NetParameter test_net_param;
      ReadProtoFromTextFile(_network_proto_filename, &test_net_param);
      
      VLOG(1) << "Initializing Caffe";
      Net<float> caffe(test_net_param);

      VLOG(1) << "Loading pretrained network from file: " << _network_filename;
      NetParameter trained_net_param;
      ReadProtoFromBinaryFile(_network_filename, &trained_net_param);
      caffe.CopyTrainedLayersFrom(trained_net_param);
      
      const int bins = GetBinsForNet(caffe);
      const int resultDepth = caffe.output_blobs()[0]->channels();
      const int planeDim = caffe.input_blobs()[0]->width();
      
      densenet_params_t params;
      params.interval = scale_intervals;
      params.img_padding = patch_size.x / 2;
      // These are determined based on the patch_size scaled to the canonical_scale.
      //params.feat_minWidth = patch_size.x * scale;
      //params.feat_minHeight = patch_size.y * scale;
      
      const uint32_t img_minHeight = params.feat_minHeight * bins;
      const uint32_t img_minWidth = params.feat_minWidth * bins;
      VLOG(1) << "Creating patchwork from image";
      Patchwork patchwork = CreatePatchwork(image, img_minWidth, img_minHeight, 
					    params.img_padding, params.interval, planeDim); 
      int nbPlanes = patchwork.planes_.size();
      
      vector<Blob<float>*> output_blobs;
      VLOG(1) << "Propagating " << nbPlanes << " patchwork planes through network";
      for (int planeID = 0; planeID < nbPlanes; planeID++) {
	const JPEGImage& currPlane = patchwork.planes_[planeID];
	
	VLOG(1) << "Level " << planeID << " Size: (" 
		<< currPlane.width() << ", " 
		<< currPlane.height() << ", " 
		<< currPlane.depth() << ")";
	
	vector<Blob<float>*> plane_input_blob(1);
	plane_input_blob[0] = new Blob<float>();
	PlaneToBlobProto(currPlane, plane_input_blob[0]);
	
	const vector<Blob<float>*> output = caffe.Forward(plane_input_blob);
	for (int i = 0; i < output.size(); i++) {
	  Blob<float>* blob = new Blob<float>();
	  blob->CopyFrom(*output[i], false, true);
	  blob->Update();
	  
	  output_blobs.push_back(blob);
	}
      }  
      
      vector<ScaleLocation> scaleLocations = unstitch_pyramid_locations(patchwork, bins);
      const int nbScales = scaleLocations.size();
      
      FeaturePyramid pyramid(nbScales);
      vector<float> scales;  
      
      VLOG(1) << "Copying features into output FeaturePyramid";
      for (int i = 0; i < nbScales; i++) {
	const int planeID = scaleLocations[i].planeID;
	ASSERT_LT(planeID, (int) output_blobs.size());
	
	Blob<float>* level_blob = output_blobs[planeID];
	const int xmin = scaleLocations[i].xMin;
	const int ymin = scaleLocations[i].yMin;
	
	const int width = scaleLocations[i].width;
	const int height = scaleLocations[i].height;
	const int channels = resultDepth;
	
	const int blob_width = level_blob->width();
	
	VLOG(1) << "Found blob at level: " << (i+1) << " [" << planeID << "]"
		<< " (" << width << ", " << height << ", " << channels << ")";
	VLOG(1) << "# Bits: " << level_blob->count();
	
	const float* bits = level_blob->cpu_data();
	FloatImage full_level(level_blob->width(), level_blob->height(), level_blob->channels());
	memcpy(full_level.data(), bits, sizeof(float) * level_blob->count());
	
	FloatImage level(width, height, channels);
	cimg_forXYZ(level, x, y, c) {
	  const int levelx = x + xmin;
	  const int levely = y + ymin;
	  level(x, y, c) = full_level(levelx, levely, c);
	}

	scales.push_back(static_cast<float>(image.width()) / static_cast<float>(level.width()));
	
	const float image_level_width = level.width();
	const float image_level_height = level.height();
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
