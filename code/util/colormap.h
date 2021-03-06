#ifndef __SLIB_UTIL_COLORMAP_H__
#define __SLIB_UTIL_COLORMAP_H__

#include <CImg.h>
#include <common/scoped_ptr.h>
#include <common/types.h>
#include <gflags/gflags.h>
#include <math.h>
#include <string/conversions.h>
#include <string>
#include <util/statistics.h>
#include <vector>

DECLARE_double(gaussian_colormap_shape);
DECLARE_double(gaussian_colormap_epsilon);

namespace slib {
  namespace util {
    
    class ColorMap {
    private:
      void SetToImage(const FloatImage& image);
      int GetIndex(const float& val) const;

      scoped_array<float> _map;
      float _map_length;
    public:
      // Loads a ColorMap from an image. The image can be any height,
      // but only the first row is used. The pixel values of the first
      // row will be mapped from 0 to 1, such that pixel (0,0) in the
      // image will map to a value of 0.
      explicit ColorMap(const std::string& filename);

      // Same as above except it takes a FloatImage.
      explicit ColorMap(const FloatImage& image);

      void Map(const float& val, float* rgb) const;
      Triplet<float> Map(const float& val) const;

      /**
	 Begin static ColorMap methods

	 These are here for convenience as commonly used colormaps
	 (based loosely on some of the more popular MATLAB colormaps).
       **/
      static void jet(const float& val, float* rgb) {
	const float map_length1 = 63.0f;
	const float jetmap[] = {0, 0, 0.5625, 0, 0, 0.6250, 0, 0, 0.6875, 0, 0, 0.7500, 0, 0, 0.8125, 0, 0, 0.8750, 0, 0, 0.9375, 0, 0, 1.0000, 0, 0.0625, 1.0000, 0, 0.1250, 1.0000, 0, 0.1875, 1.0000, 0, 0.2500, 1.0000, 0, 0.3125, 1.0000, 0, 0.3750, 1.0000, 0, 0.4375, 1.0000, 0, 0.5000, 1.0000, 0, 0.5625, 1.0000, 0, 0.6250, 1.0000, 0, 0.6875, 1.0000, 0, 0.7500, 1.0000, 0, 0.8125, 1.0000, 0, 0.8750, 1.0000, 0, 0.9375, 1.0000, 0, 1.0000, 1.0000, 0.0625, 1.0000, 0.9375, 0.1250, 1.0000, 0.8750, 0.1875, 1.0000, 0.8125, 0.2500, 1.0000, 0.7500, 0.3125, 1.0000, 0.6875, 0.3750, 1.0000, 0.6250, 0.4375, 1.0000, 0.5625, 0.5000, 1.0000, 0.5000, 0.5625, 1.0000, 0.4375, 0.6250, 1.0000, 0.3750, 0.6875, 1.0000, 0.3125, 0.7500, 1.0000, 0.2500, 0.8125, 1.0000, 0.1875, 0.8750, 1.0000, 0.1250, 0.9375, 1.0000, 0.0625, 1.0000, 1.0000, 0, 1.0000, 0.9375, 0, 1.0000, 0.8750, 0, 1.0000, 0.8125, 0, 1.0000, 0.7500, 0, 1.0000, 0.6875, 0, 1.0000, 0.6250, 0, 1.0000, 0.5625, 0, 1.0000, 0.5000, 0, 1.0000, 0.4375, 0, 1.0000, 0.3750, 0, 1.0000, 0.3125, 0, 1.0000, 0.2500, 0, 1.0000, 0.1875, 0, 1.0000, 0.1250, 0, 1.0000, 0.0625, 0, 1.0000, 0, 0, 0.9375, 0, 0, 0.8750, 0, 0, 0.8125, 0, 0, 0.7500, 0, 0, 0.6875, 0, 0, 0.6250, 0, 0, 0.5625, 0, 0, 0.5000, 0, 0};
	float lookup = val < 0 ? 0 : val;
	lookup = lookup > 1 ? 1 : lookup;
	if (val != val) {
	  lookup = 0;
	}

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

      static void bluehot(const float& val, float* rgb) {
	const float map_length1 = 99.0f;
	const float map[] = {0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.15152, 0.00000, 0.00000, 0.30051, 0.00000, 0.00000, 0.32576, 0.00000, 0.00000, 0.35101, 0.00000, 0.00000, 0.37626, 0.00000, 0.00000, 0.40152, 0.00000, 0.00000, 0.42677, 0.00000, 0.00000, 0.45202, 0.00000, 0.00000, 0.47727, 0.00000, 0.00000, 0.50253, 0.00000, 0.00000, 0.52778, 0.00000, 0.00000, 0.55303, 0.00000, 0.00000, 0.57828, 0.00000, 0.00000, 0.60354, 0.00000, 0.00000, 0.62879, 0.00000, 0.00000, 0.65404, 0.00000, 0.00000, 0.67929, 0.00000, 0.00000, 0.70455, 0.00000, 0.00000, 0.72980, 0.00000, 0.00000, 0.75505, 0.00000, 0.00000, 0.78030, 0.00000, 0.00000, 0.80556, 0.00000, 0.00000, 0.83081, 0.00000, 0.00000, 0.85606, 0.00000, 0.00000, 0.88131, 0.00000, 0.00000, 0.90657, 0.00000, 0.00000, 0.93182, 0.00000, 0.00000, 0.95707, 0.00000, 0.00000, 0.98232, 0.00000, 0.00758, 1.00000, 0.00000, 0.03283, 1.00000, 0.00000, 0.05808, 1.00000, 0.00000, 0.08333, 1.00000, 0.00000, 0.10859, 1.00000, 0.00000, 0.13384, 1.00000, 0.00000, 0.15909, 1.00000, 0.00000, 0.18434, 1.00000, 0.00000, 0.20960, 1.00000, 0.00000, 0.23485, 1.00000, 0.00000, 0.26010, 1.00000, 0.00000, 0.28535, 1.00000, 0.00000, 0.31061, 1.00000, 0.00000, 0.33586, 1.00000, 0.00000, 0.36111, 1.00000, 0.00000, 0.38636, 1.00000, 0.00000, 0.41162, 1.00000, 0.00000, 0.43687, 1.00000, 0.00000, 0.46212, 1.00000, 0.00000, 0.48737, 1.00000, 0.00000, 0.51263, 1.00000, 0.00000, 0.53788, 1.00000, 0.00000, 0.56313, 1.00000, 0.00000, 0.58838, 1.00000, 0.00000, 0.61364, 1.00000, 0.00000, 0.63889, 1.00000, 0.00000, 0.66414, 1.00000, 0.00000, 0.68939, 1.00000, 0.00000, 0.71465, 1.00000, 0.00000, 0.73990, 1.00000, 0.00000, 0.76515, 1.00000, 0.00000, 0.79040, 1.00000, 0.00000, 0.81566, 1.00000, 0.00000, 0.84091, 1.00000, 0.00000, 0.86616, 1.00000, 0.00000, 0.89141, 1.00000, 0.00000, 0.91667, 1.00000, 0.00000, 0.94192, 1.00000, 0.00000, 0.96717, 1.00000, 0.00000, 0.99242, 1.00000, 0.02357, 1.00000, 1.00000, 0.05724, 1.00000, 1.00000, 0.09091, 1.00000, 1.00000, 0.12458, 1.00000, 1.00000, 0.15825, 1.00000, 1.00000, 0.19192, 1.00000, 1.00000, 0.22559, 1.00000, 1.00000, 0.25926, 1.00000, 1.00000, 0.29293, 1.00000, 1.00000, 0.32660, 1.00000, 1.00000, 0.36027, 1.00000, 1.00000, 0.39394, 1.00000, 1.00000, 0.42761, 1.00000, 1.00000, 0.46128, 1.00000, 1.00000, 0.49495, 1.00000, 1.00000, 0.52862, 1.00000, 1.00000, 0.56229, 1.00000, 1.00000, 0.59596, 1.00000, 1.00000, 0.62963, 1.00000, 1.00000, 0.66330, 1.00000, 1.00000, 0.69697, 1.00000, 1.00000, 0.73064, 1.00000, 1.00000, 0.76431, 1.00000, 1.00000, 0.79798, 1.00000, 1.00000, 0.83165, 1.00000, 1.00000, 0.86532, 1.00000, 1.00000, 0.89899, 1.00000, 1.00000, 0.93266, 1.00000, 1.00000, 0.96633, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000};
	float lookup = val < 0 ? 0 : val;
	lookup = lookup > 1 ? 1 : lookup;
	if (val != val) {
	  lookup = 0;
	}

	const int32 idx = static_cast<int32>(lookup * map_length1);
	rgb[0] = map[idx * 3];
	rgb[1] = map[idx * 3 + 1];
	rgb[2] = map[idx * 3 + 2];
      }

      static void redhot(const float& val, float* rgb) {
	const float map_length1 = 99.0f;
	const float map[] = {0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.15152, 0.00000, 0.00000, 0.30051, 0.00000, 0.00000, 0.32576, 0.00000, 0.00000, 0.35101, 0.00000, 0.00000, 0.37626, 0.00000, 0.00000, 0.40152, 0.00000, 0.00000, 0.42677, 0.00000, 0.00000, 0.45202, 0.00000, 0.00000, 0.47727, 0.00000, 0.00000, 0.50253, 0.00000, 0.00000, 0.52778, 0.00000, 0.00000, 0.55303, 0.00000, 0.00000, 0.57828, 0.00000, 0.00000, 0.60354, 0.00000, 0.00000, 0.62879, 0.00000, 0.00000, 0.65404, 0.00000, 0.00000, 0.67929, 0.00000, 0.00000, 0.70455, 0.00000, 0.00000, 0.72980, 0.00000, 0.00000, 0.75505, 0.00000, 0.00000, 0.78030, 0.00000, 0.00000, 0.80556, 0.00000, 0.00000, 0.83081, 0.00000, 0.00000, 0.85606, 0.00000, 0.00000, 0.88131, 0.00000, 0.00000, 0.90657, 0.00000, 0.00000, 0.93182, 0.00000, 0.00000, 0.95707, 0.00000, 0.00000, 0.98232, 0.00000, 0.00758, 1.00000, 0.00000, 0.03283, 1.00000, 0.00000, 0.05808, 1.00000, 0.00000, 0.08333, 1.00000, 0.00000, 0.10859, 1.00000, 0.00000, 0.13384, 1.00000, 0.00000, 0.15909, 1.00000, 0.00000, 0.18434, 1.00000, 0.00000, 0.20960, 1.00000, 0.00000, 0.23485, 1.00000, 0.00000, 0.26010, 1.00000, 0.00000, 0.28535, 1.00000, 0.00000, 0.31061, 1.00000, 0.00000, 0.33586, 1.00000, 0.00000, 0.36111, 1.00000, 0.00000, 0.38636, 1.00000, 0.00000, 0.41162, 1.00000, 0.00000, 0.43687, 1.00000, 0.00000, 0.46212, 1.00000, 0.00000, 0.48737, 1.00000, 0.00000, 0.51263, 1.00000, 0.00000, 0.53788, 1.00000, 0.00000, 0.56313, 1.00000, 0.00000, 0.58838, 1.00000, 0.00000, 0.61364, 1.00000, 0.00000, 0.63889, 1.00000, 0.00000, 0.66414, 1.00000, 0.00000, 0.68939, 1.00000, 0.00000, 0.71465, 1.00000, 0.00000, 0.73990, 1.00000, 0.00000, 0.76515, 1.00000, 0.00000, 0.79040, 1.00000, 0.00000, 0.81566, 1.00000, 0.00000, 0.84091, 1.00000, 0.00000, 0.86616, 1.00000, 0.00000, 0.89141, 1.00000, 0.00000, 0.91667, 1.00000, 0.00000, 0.94192, 1.00000, 0.00000, 0.96717, 1.00000, 0.00000, 0.99242, 1.00000, 0.02357, 1.00000, 1.00000, 0.05724, 1.00000, 1.00000, 0.09091, 1.00000, 1.00000, 0.12458, 1.00000, 1.00000, 0.15825, 1.00000, 1.00000, 0.19192, 1.00000, 1.00000, 0.22559, 1.00000, 1.00000, 0.25926, 1.00000, 1.00000, 0.29293, 1.00000, 1.00000, 0.32660, 1.00000, 1.00000, 0.36027, 1.00000, 1.00000, 0.39394, 1.00000, 1.00000, 0.42761, 1.00000, 1.00000, 0.46128, 1.00000, 1.00000, 0.49495, 1.00000, 1.00000, 0.52862, 1.00000, 1.00000, 0.56229, 1.00000, 1.00000, 0.59596, 1.00000, 1.00000, 0.62963, 1.00000, 1.00000, 0.66330, 1.00000, 1.00000, 0.69697, 1.00000, 1.00000, 0.73064, 1.00000, 1.00000, 0.76431, 1.00000, 1.00000, 0.79798, 1.00000, 1.00000, 0.83165, 1.00000, 1.00000, 0.86532, 1.00000, 1.00000, 0.89899, 1.00000, 1.00000, 0.93266, 1.00000, 1.00000, 0.96633, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000};
	float lookup = val < 0 ? 0 : val;
	lookup = lookup > 1 ? 1 : lookup;
	if (val != val) {
	  lookup = 0;
	}

	const int32 idx = static_cast<int32>(lookup * map_length1);
	rgb[0] = map[idx * 3 + 2];
	rgb[1] = map[idx * 3 + 1];
	rgb[2] = map[idx * 3];
      }

      static void hot(const float& val, float* rgb) {
	const float map_length1 = 63.0f;
	float r[] = { 0, 0.03968253968253968, 0.07936507936507936, 0.119047619047619, 0.1587301587301587, 0.1984126984126984, 0.2380952380952381, 0.2777777777777778, 0.3174603174603174, 0.3571428571428571, 0.3968253968253968, 0.4365079365079365, 0.4761904761904762, 0.5158730158730158, 0.5555555555555556, 0.5952380952380952, 0.6349206349206349, 0.6746031746031745, 0.7142857142857142, 0.753968253968254, 0.7936507936507936, 0.8333333333333333, 0.873015873015873, 0.9126984126984127, 0.9523809523809523, 0.992063492063492, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
	float g[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.03174603174603163, 0.0714285714285714, 0.1111111111111112, 0.1507936507936507, 0.1904761904761905, 0.23015873015873, 0.2698412698412698, 0.3095238095238093, 0.3492063492063491, 0.3888888888888888, 0.4285714285714284, 0.4682539682539679, 0.5079365079365079, 0.5476190476190477, 0.5873015873015872, 0.6269841269841268, 0.6666666666666665, 0.7063492063492065, 0.746031746031746, 0.7857142857142856, 0.8253968253968254, 0.8650793650793651, 0.9047619047619047, 0.9444444444444442, 0.984126984126984, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
	float b[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.04761904761904745, 0.1269841269841265, 0.2063492063492056, 0.2857142857142856, 0.3650793650793656, 0.4444444444444446, 0.5238095238095237, 0.6031746031746028, 0.6825396825396828, 0.7619047619047619, 0.8412698412698409, 0.92063492063492, 1};

	float lookup = val < 0 ? 0 : val;
	lookup = lookup > 1 ? 1 : lookup;
	if (val != val) {
	  lookup = 0;
	}

	const int32 idx = static_cast<int32>(lookup * map_length1);
	rgb[0] = r[idx];
	rgb[1] = g[idx];
	rgb[2] = b[idx];
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
