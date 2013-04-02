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

DECLARE_double(gaussian_colormap_shape);
DECLARE_double(gaussian_colormap_epsilon);

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
      static void RGBToHSV(const T* rgb, T* hsv) {
	T min = rgb[0] < rgb[1] ? rgb[0] : rgb[1];
	min = min  < rgb[2] ? min  : rgb[2];
	
	T max = rgb[0] > rgb[1] ? rgb[0] : rgb[1];
	max = max  > rgb[2] ? max  : rgb[2];
	
	hsv[2] = max;
	T delta = max - min;
	if (max > 0) {
	  hsv[1] = (delta / max);
	} else {
	  hsv[1] = 0;
	  hsv[0] = 0;
	  return;
	}
	if (rgb[0] >= max) {
	  hsv[0] = (rgb[1] - rgb[2]) / delta;
	} else {
	  if (rgb[1] >= max) {
	    hsv[0] = 2 + (rgb[2] - rgb[0]) / delta;
	  } else {
	    hsv[0] = 4 + (rgb[0] - rgb[1]) / delta;
	  }
	}
	
	hsv[0] *= 60;
	
	if (hsv[0] < 0) {
	  hsv[0] += 360;
	}
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
					  const float epsilon = 0.4);
      static float InverseGaussianTransformation(const float& max, 
						 const float& min, 
						 const float& val,
						 const float shape = -1,
						 const float epsilon = 0.4);
    };    
  } // namespace util
} // namespace slib
#endif 
