#ifndef __SLIB_UTIL_COLORMAP_H__
#define __SLIB_UTIL_COLORMAP_H__

#include <CImg.h>
#include "../common/types.h"
#include <gflags/gflags.h>
#include <math.h>
#include "statistics.h"
#include "../string/conversions.h"
#include <string>
#include <vector>

namespace slib {
  namespace util {
    
    class ColorMap {
    private:
    public:
      static void jet(const float& val, float* rgb) {
	const float map_length1 = 63.0f;
	const float jetmap[] = {0, 0, 0.5625, 0, 0, 0.6250, 0, 0, 0.6875, 0, 0, 0.7500, 0, 0, 0.8125, 0, 0, 0.8750, 0, 0, 0.9375, 0, 0, 1.0000, 0, 0.0625, 1.0000, 0, 0.1250, 1.0000, 0, 0.1875, 1.0000, 0, 0.2500, 1.0000, 0, 0.3125, 1.0000, 0, 0.3750, 1.0000, 0, 0.4375, 1.0000, 0, 0.5000, 1.0000, 0, 0.5625, 1.0000, 0, 0.6250, 1.0000, 0, 0.6875, 1.0000, 0, 0.7500, 1.0000, 0, 0.8125, 1.0000, 0, 0.8750, 1.0000, 0, 0.9375, 1.0000, 0, 1.0000, 1.0000, 0.0625, 1.0000, 0.9375, 0.1250, 1.0000, 0.8750, 0.1875, 1.0000, 0.8125, 0.2500, 1.0000, 0.7500, 0.3125, 1.0000, 0.6875, 0.3750, 1.0000, 0.6250, 0.4375, 1.0000, 0.5625, 0.5000, 1.0000, 0.5000, 0.5625, 1.0000, 0.4375, 0.6250, 1.0000, 0.3750, 0.6875, 1.0000, 0.3125, 0.7500, 1.0000, 0.2500, 0.8125, 1.0000, 0.1875, 0.8750, 1.0000, 0.1250, 0.9375, 1.0000, 0.0625, 1.0000, 1.0000, 0, 1.0000, 0.9375, 0, 1.0000, 0.8750, 0, 1.0000, 0.8125, 0, 1.0000, 0.7500, 0, 1.0000, 0.6875, 0, 1.0000, 0.6250, 0, 1.0000, 0.5625, 0, 1.0000, 0.5000, 0, 1.0000, 0.4375, 0, 1.0000, 0.3750, 0, 1.0000, 0.3125, 0, 1.0000, 0.2500, 0, 1.0000, 0.1875, 0, 1.0000, 0.1250, 0, 1.0000, 0.0625, 0, 1.0000, 0, 0, 0.9375, 0, 0, 0.8750, 0, 0, 0.8125, 0, 0, 0.7500, 0, 0, 0.6875, 0, 0, 0.6250, 0, 0, 0.5625, 0, 0, 0.5000, 0, 0};
	float lookup = val < 0 ? 0 : val;
	lookup = lookup > 1 ? 1 : lookup;

	const int32 idx = static_cast<int32>(lookup * map_length1);
	rgb[0] = jetmap[idx * 3];
	rgb[1] = jetmap[idx * 3 + 1];
	rgb[2] = jetmap[idx * 3 + 2];
      }
      
      template <typename T>
      static bool HexToRGB(const std::string& hex, 
			   T* rgb, const T& maxval = 1) {
	int offset = 0;
	if (hex[0] == '#') {
	  offset = 1;
	} else if (hex[0] == '0' && hex[1] == 'x') {
	  offset = 2;
	} else if (hex.length() != 6) {
	  return false;
	}

	const std::string red = hex.substr(offset, 2);
	const std::string green = hex.substr(offset+2, 2);
	const std::string blue = hex.substr(offset+4, 2);
	
	int value;

	if (!slib::string::ParseHexString(red, &value)) {
	  return false;
	}
	rgb[0] = static_cast<T>(value) / maxval;

	if (!slib::string::ParseHexString(green, &value)) {
	  return false;
	}
	rgb[1] = static_cast<T>(value) / maxval;

	if (!slib::string::ParseHexString(blue, &value)) {
	  return false;
	}
	rgb[2] = static_cast<T>(value) / maxval;

	return true;
      }

      static float GaussianTransformation(const float& max, 
					  const float& min, 
					  const float& value,
					  const float shape = -1,
					  const float epsilon = 0.4) {
	float val = 0.0f;
	if (shape > 0.0) {
	  const float v = (value - min) / (max - min);
	  const float p = 1.0f - v;
	  val = exp(-pow(p, shape) / (2.0 * epsilon));
	} else {
	  val = (value - min) / (max - min);
	}
	val = val < 0 ? 0 : val;
	val = val > 1 ? 1 : val;

	return val;
      }

      static float InverseGaussianTransformation(const float& max, 
						 const float& min, 
						 const float& val,
						 const float shape = -1,
						 const float epsilon = 0.4) {
	float value = 0.0f;
	if (shape > 0.0) {
	  value = min + (max - min) * 
	    (1.0f - pow(-2.0f * epsilon * log(val), 1/shape));
	} else {
	  value = min + val * (max - min);
	}

	return value;
      }

      template <typename T>
      static void OverlayLegend(const std::vector<T>& transformed_values, 
				const std::vector<T>& original_values,
				void (*colormap_function)(const float&, float*), 
				float (*inverse_function)(const float&, const float&, const float&), 
				FloatImage* image) {
	const int32 N = transformed_values.size();
	float* transformed_values_ptr = new float[N];
	for (uint32 i = 0; i < N; i++) {
	  transformed_values_ptr[i] = static_cast<float>(transformed_values[i]);
	}
	const float max_transformed_value = Max<float>(transformed_values_ptr, N);
	const float min_transformed_value = Min<float>(transformed_values_ptr, N);

	const int32 M = original_values.size();
	float* original_values_ptr = new float[N];
	for (uint32 i = 0; i < N; i++) {
	  original_values_ptr[i] = static_cast<float>(original_values[i]);
	}
	const float max_original_value = Max<float>(original_values_ptr, N);
	const float min_original_value = Min<float>(original_values_ptr, N);
	
	float max_color[3];
	float min_color[3];
	(*colormap_function)(max_transformed_value, max_color);
	(*colormap_function)(min_transformed_value, min_color);
	
	// Resize the image to accomodate the legend.
	const int32 legend_height = 0.1 * image->height();
	const float color[3] = { 51.0f, 51.0f, 51.0f };
	image->resize(image->width(), image->height() + legend_height,
		      image->depth(), image->spectrum(), 0);
	image->draw_image(0, legend_height, 0, 0, *image);
	image->draw_rectangle(0, 0, image->width(), legend_height, color);

	// Draw the colormap on the legend.
	const float white[3] = {255.0f, 255.0f, 255.0f};
	float rgb[3];
	const int32 line_height = legend_height * 0.4;
	const int32 line_y = (0.5f * (float) (legend_height - line_height));
	const int32 colormap_padding = image->width() * 0.03;
	const int32 num_ticks = 10;
	const int32 tick_separation = ceil(((float) (image->width() - 2*colormap_padding)) / ((float) num_ticks));
	const int32 font_height = line_y * 0.5;
	for (int x = colormap_padding; x < image->width()-colormap_padding; x++) {	  
	  const float x_norm = static_cast<float>(x-colormap_padding) 
	    / static_cast<float>(image->width()-colormap_padding);
	  const float val = x_norm * (max_transformed_value - min_transformed_value) + min_transformed_value;
	  (*colormap_function)(val, rgb);
	  for (int c = 0; c < 3; c++) {
	    rgb[c] = rgb[c] * 255.0f;
	  }
	  image->draw_line(x, line_y, x, line_height, rgb);

	  if ((x - colormap_padding) % tick_separation == 0) {
	    image->draw_line(x, line_y-10, x, line_height+10, white);

	    const float point_value = (*inverse_function)(max_original_value, min_original_value, val);

	    char str[255];
	    sprintf(str, "%.2f", point_value);
	    image->draw_text(x - font_height, 
			     line_y + line_height + 10 + 5 - line_y*0.5, 
			     str, 
			     white, 0, 1, font_height);
	  }
	}

	// The last line.
	const int32 x = image->width() - colormap_padding;
	image->draw_line(x, line_y-10, x, line_height+10, white);	
	const float point_value = max_original_value;
	char str[255];
	sprintf(str, "%.2f", point_value);
	image->draw_text(x - font_height, 
			 line_y + line_height + 10 + 5 - line_y*0.5, 
			 str, 
			 white, 0, 1, font_height);
	
	delete[] transformed_values_ptr;
	delete[] original_values_ptr;
      }
    };    
  } // namespace util
} // namespace slib
#endif 
