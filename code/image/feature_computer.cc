#include "feature_computer.h"

#include <algorithm>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <image/feature_pyramid.h>
#include <string>
#include <vector>

using cimg_library::CImgList;
using std::string;
using std::vector;

namespace slib {
  namespace image {

    FloatImage FeatureComputer::ComputeFeatures(const std::string& image_filename) const {
      const FloatImage image(image_filename.c_str());
      return ComputeFeatures(image);
    }

    Pair<float> FeatureComputer::GetPatchSize(const Pair<float>& canonical_patch_size) const {
      return canonical_patch_size;
    }

    FeaturePyramid FeatureComputer::ComputeFeaturePyramid(const string& image_filename, 
							  const float& image_canonical_size,
							  const int32& scale_intervals, 
							  const Pair<int32>& patch_size,
							  const vector<int32>& levels) const {
      const FloatImage image(image_filename.c_str());
      return ComputeFeaturePyramid(image, image_canonical_size, scale_intervals, patch_size, levels);
    }

    FeaturePyramid FeatureComputer::ComputeFeaturePyramid(const FloatImage& image, 
							  const float& image_canonical_size,
							  const int32& scale_intervals, 
							  const Pair<int32>& patch_size,
							  const vector<int32>& levels) const {
      VLOG(1) << "Original Image Size: " << image.width() << " x " << image.height();
      // Image should have pixels in [0,1]
      if (image.max() > 1.0f) {
	LOG(ERROR) << "Image pixels must lie in the domain [0,1] (Max: " << image.max() << ")";
	return FeaturePyramid(0);
      }
            
      // Determine number of pyramid levels for the image.
      float scale = 0.0f;
      if (image.width() < image.height()) {
	scale = image_canonical_size / static_cast<float>(image.width());
      } else {
	scale = image_canonical_size / static_cast<float>(image.height());
      }

      if (image_canonical_size <= 0) {
	scale = 1.0f;
      }

      VLOG(1) << "Canonical Scale: " << scale;
      
      const float scaled_width = scale * image.width();
      const float scaled_height = scale * image.height();
      
      const int32 level1 
	= (int32) floor((float) scale_intervals 
			* log2(scaled_width / ((float) patch_size.x)));
      const int32 level2 
	= (int32) floor((float) scale_intervals 
			* log2(scaled_height / ((float) patch_size.y)));
      const int32 num_levels = std::min(level1, level2) + 1;
      VLOG(1) << "Number of levels in image: " << num_levels;
      
      vector<int32> levels_to_compute(levels);
      if (levels_to_compute.size() == 0) {
	for (int i = 0; i < num_levels; i++) {
	  levels_to_compute.push_back(i);
	}
      }
      
      // Determine the number of scales for the image at each level.
      const float sc = pow(2.0f, 1.0f / ((float) scale_intervals));
      scoped_array<float> scales(new float[num_levels]);
      for (int i = 0; i < num_levels; i++) {
	scales[i] = pow(sc, (float) i);
	VLOG(2) << "Scale for level " << i << ": " << scales[i];
      }
      
      const int32 num_bins = 11;
      scoped_array<int32> bins(new int32[num_bins]);
      for (int i = 0; i < num_bins; i++) {
	bins[i] = -100 + i*20;
	if (i == num_bins - 1) {
	  bins[i]++;
	}
      }
      
      ASSERT_LTE((int32) levels_to_compute.size(), num_levels);
      
      FeaturePyramid pyramid(num_levels);
      int32 numx;
      int32 numy;

      // Compute feature for each level in the feature pyramid.
      for (uint32 i = 0; i < levels_to_compute.size(); i++) {
	const int32 level = levels_to_compute[i];
	const float level_scale = scale / scales[level];
	VLOG(1) << "Level Scale: " << level_scale;
	
	const float image_level_width = ceil(level_scale * ((float) image.width()));
	const float image_level_height = ceil(level_scale * ((float) image.height()));
	FloatImage image_level = image.get_resize(image_level_width, image_level_height,
						  1, image.spectrum(), 5);
	VLOG(1) << "Image Level Size: " << image_level.width() << " x " << image_level.height();
		
	const FloatImage features = ComputeFeatures(image_level);
	numx = features.width();
	numy = features.height();
	
	const FloatImage gradient_magnitude = ComputeGradientMagnitude(image_level, numx, numy);

	// Save into pyramid.
	pyramid.AddLevel(level, features);
	pyramid.AddGradientLevel(level, gradient_magnitude);
	
	VLOG(1) << "Level " << level << " Size: " << numx << " x " << numy;
      }
      // Set the parameters of the pyramid.
      pyramid.SetCanonicalScale(scale);
      pyramid.SetCanonicalSize(Pair<int32>(numx, numy));
      pyramid.SetScales(scales.get());
      pyramid.SetOriginalImageSize(Pair<int32>(image.width(), image.height()));
      
      return pyramid;
    }

    FloatImage FeatureComputer::ComputeGradientMagnitude(const FloatImage& image, 
							 const int& output_width, 
							 const int& output_height) const {
      // Compute the gradient of this level's image. For options to this
      // method see:
      // http://cimg.sourceforge.net/reference/structcimg__library_1_1CImg.html#a3e5b54c0b862cbf6e9f14e832984c4d7
      CImgList<float> gradient = image.get_gradient("xy", 0);
      if (FLAGS_v >= 3) {
	image.display();
	(gradient[0], gradient[1]).display();
      }
      // Compute the magnitude of the gradient.
      FloatImage gradient_magnitude(gradient[0].width(), gradient[0].height());
      {
	FloatImage gradient_magnitude_3 = (gradient[0] * 255.0f).sqr() + (gradient[1] * 255.0f).sqr();
	cimg_forXY(gradient_magnitude, x, y) {
	  float val = 0.0f;
	  cimg_forC(gradient[0], c) {
	    val += gradient_magnitude_3(x, y, c);
	  }
	  gradient_magnitude(x, y) = val / 3.0f;
	}
      }
      // Downsample to the correct size.
      gradient_magnitude.resize(output_width, output_height, -100, -100, 3);  // Bilinear
      if (FLAGS_v >= 3) {
	gradient_magnitude.display();
      }

      return gradient_magnitude;
    }
  }  // namespace image
}  // namespace slib
