#include "color_histogram_feature_computer.h"

#include <algorithm>
#include <CImg.h>
#include <common/types.h>
#include <image/feature_pyramid.h>
#include <vector>

  // These are the bounds of the Lab space according to ~/Development/sandbox/lab/labbounds.
#define MIN_LAB_L -100.0
#define MAX_LAB_L +100.0
#define MIN_LAB_A -86.181252
#define MAX_LAB_A +98.235161
#define MIN_LAB_B -107.861755
#define MAX_LAB_B +94.475792

DEFINE_int32(color_histogram_feature_computer_color_bins, 32, "The number of bins to use for colors.");

using std::vector;

namespace slib {
  namespace image {

    ColorHistogramFeatureComputer::ColorHistogramFeatureComputer(const int& spatial_bins) 
      : _spatial_bins(spatial_bins) {}

    FloatImage ColorHistogramFeatureComputer::ComputeFeatures(const FloatImage& image) const {
      LOG(ERROR) << "Do not call ComputeFeatures directly. "
		 << "Use the ComputeFeaturePyramid(...) instead.";
      return FloatImage();
    }

    int ColorHistogramFeatureComputer::GetPatchChannels() {
      return FLAGS_color_histogram_feature_computer_color_bins * 2;
    }

    FeaturePyramid ColorHistogramFeatureComputer::ComputeFeaturePyramid(const FloatImage& image, 
									const float& image_canonical_size,
									const int32& scale_intervals, 
									const Pair<int32>& patch_size,
									const std::vector<int32>& levels) const {
      if (image.max() > 1.0f) {
	LOG(ERROR) << "Image pixels must lie in the domain [0,1] (Max: " << image.max() << ")";
	return FeaturePyramid(0);
      }

      if (image.spectrum() != 3) {
	LOG(ERROR) << "Image must be an RGB image";
	return FeaturePyramid(0);
      }

      VLOG(1) << "Original Image Size: " << image.width() << " x " << image.height();

      // Scale the image to the canonical size.
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

      const Pair<int32> image_size(image.width(), image.height());      

      const int32 level1 = (int32) floor((float) scale_intervals * log2(scaled_width / ((float) patch_size.x)));
      const int32 level2 = (int32) floor((float) scale_intervals * log2(scaled_height / ((float) patch_size.y)));
      const int32 num_levels = std::min(level1, level2) + 1;
      VLOG(1) << "Number of levels in image: " << num_levels;
            
      // Determine the number of scales for the image at each level.
      const float sc = pow(2.0f, 1.0f / ((float) scale_intervals));
      vector<float> scales(num_levels);
      for (int i = 0; i < num_levels; i++) {
	scales[i] = pow(sc, (float) i);
	VLOG(2) << "Scale for level " << i << ": " << scales[i];
      }

      FeaturePyramid pyramid(num_levels);
      const int color_bins = FLAGS_color_histogram_feature_computer_color_bins;
      for (uint32 i = 0; i < num_levels; i++) {
	const int32 level = i;
	const float level_scale = scale / scales[level];
	VLOG(1) << "Scale: " << scales[level];
	VLOG(1) << "Level Scale: " << level_scale;

	const float image_level_width = ceil(level_scale * ((float) image.width()));
	const float image_level_height = ceil(level_scale * ((float) image.height()));
	FloatImage image_level = image.get_resize(image_level_width, image_level_height, -100, -100, 5);
	VLOG(1) << "Image Level Size: " << image_level.width() << " x " << image_level.height();
	
	// Truncate the image to fit exactly within the bounds of the bins.
	const int32 overflow_x = image_level.width() % _spatial_bins;
	const int32 overflow_y = image_level.height() % _spatial_bins;
	if (overflow_x > 0 || overflow_y > 0) {
	  image_level.crop(0, 0, image_level.width() - overflow_x - 1, image_level.height() - overflow_y - 1);
	  VLOG(1) << "Cropping to: " << image_level.width() << " x " << image_level.height();
	}

	// Convert to Lab.
	FloatImage lab_image(image_level * 255);  // Conversions expect [0, 255]
	lab_image.RGBtoLab();
	
	const int fw = image_level.width() / _spatial_bins;
	const int fh = image_level.height() / _spatial_bins;
	FloatImage features(fw, fh, color_bins * 2);

	for (int y = 0; y < image_level.height(); y += _spatial_bins) {
	  for (int x = 0; x < image_level.width(); x += _spatial_bins) {
	    const FloatImage patch = lab_image.get_crop(x, y, x + _spatial_bins - 1, y + _spatial_bins - 1);
	    const FloatImage a_hist = patch.get_channel(1).histogram(color_bins, MIN_LAB_A, MAX_LAB_A);
	    const FloatImage b_hist = patch.get_channel(2).histogram(color_bins, MIN_LAB_B, MAX_LAB_B);

	    ASSERT_EQ(a_hist.width(), color_bins);
	    ASSERT_EQ(b_hist.width(), color_bins);

	    const int fx = x / _spatial_bins;
	    const int fy = y / _spatial_bins;
	    memcpy(features.data() + (fx + fy * fw) * 2 * color_bins, 
		   a_hist.data(), sizeof(float) * color_bins);
	    memcpy(features.data() + (fx + fy * fw) * 2 * color_bins + color_bins, 
		   b_hist.data(), sizeof(float) * color_bins);
	  }
	}

	const FloatImage gradient_magnitude 
	  = ComputeGradientMagnitude(image_level, image_level.width(), image_level.height());
	
	pyramid.AddLevel(i, features);
	pyramid.AddGradientLevel(i, gradient_magnitude);
      }

      pyramid.SetCanonicalScale(scale);
      pyramid.SetCanonicalSize(Pair<int32>(0, 0));
      pyramid.SetScales(scales);
      pyramid.SetOriginalImageSize(image_size);
      
      return pyramid;
    }

  }  // namespace image
}  // namespace slib
