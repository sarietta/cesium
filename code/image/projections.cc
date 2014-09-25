#include "projections.h"

#include <CImg.h>
#include <common/types.h>
#include <math.h>

namespace slib {
  namespace image {
    
    void PerspectiveProjection(const FloatImage& image, FloatImage* rectified_image, 
			       const float& fov, const float& heading, 
			       const float& pitch, const bool& use_cubic_interpolation) {
      const float width = image.width();
      const float height = image.height();
      
      const float output_width = rectified_image->width();
      const float output_height = rectified_image->height();
      
      const float heading_radians = -heading * M_PI / 180.0f;
      const float cos_heading = cos(heading_radians);
      const float sin_heading = sin(heading_radians);
      
      const float pitch_radians = -pitch * M_PI / 180.0f;
      const float cos_pitch = cos(pitch_radians);
      const float sin_pitch = sin(pitch_radians);
      
      const float fov_radians = fov * M_PI / 180.0f;
      const float x = 1.0f / tan(fov_radians / 2.0f);
      
      // The image is defined in the z-y plane.
      cimg_forXY(*rectified_image, i, j) {
	//for (int j = 0; j < rectified_image->height(); j++) {
	const float z = -(2.0f * static_cast<float>(j) / (output_height) - 1.0f);
	//for (int i = 0; i < rectified_image->width(); i++) {
	const float y = -(2.0f * static_cast<float>(i) / (output_width) - 1.0f);
	
	// Rotate
	const float xr = 
	  cos_heading * cos_pitch * x 
	  - sin_heading * y + cos_heading * sin_pitch * z;
	const float yr = 
	  sin_heading * cos_pitch * x 
	  + cos_heading * y + sin_heading * sin_pitch * z;
	const float zr = -sin_pitch * x + cos_pitch * z;
	// Compute spherical coordinates.
	const float r = sqrt(xr*xr + yr*yr + zr*zr);
	const float theta = atan2(xr, yr);
	const float phi = acos(zr / r);
	
	// Compute lookup in original image. Technically, we are
	// dropping the r coordinate to 1, but it doesn't appear in
	// these calculations anyway. It's worth noting that however,
	// because the above calculation actually computes the spherical
	// coordinates of the point on a sphere of radius r. Since we
	// want a sphere of radius 1, we drop r to 1, but that doesn't
	// effect the angular coordinates.
	int image_i = static_cast<int>(theta * M_1_PI * width * 0.5);
	int image_j = static_cast<int>(phi * M_1_PI * height);
	if (image_i < 0) {
	  image_i += width;
	} else if (image_i >= width) {
	  image_i -= width;
	}
	if (image_j < 0) {
	  continue;
	  image_j += height;
	} else if (image_j >= height) {
	  continue;
	  image_j -= height;
	}
	// Copy pixel.
	if (use_cubic_interpolation) {
	  for (int c = 0; c < image.spectrum(); c++) {
	    (*rectified_image)(i,j,0,c) 
	      = image.cubic_atXY(image_i, image_j, 0, c, 0.0f);
	  }
	} else {
	  cimg_forC(*rectified_image, c) {
	    (*rectified_image)(i,j,0,c) = image(image_i, image_j, 0, c);
	  }
	}
      }
    }
    
    void StereographicProjection(const FloatImage& image, 
				 FloatImage* rectified_image, const float& z) {
      const float width = image.width();
      const float height = image.height();
      for (int j = 0; j < rectified_image->height(); j++) {
	const float y = (2.0f * static_cast<float>(j) 
			 / (rectified_image->height()) - 1.0f);
	for (int i = 0; i < rectified_image->width(); i++) {
	  const float x = (2.0f * static_cast<float>(i) 
			   / (rectified_image->width()) - 1.0f);
	  
	  // Singularity.
	  if (x == 0 && y == 0) {
	    continue;
	  }
	  
	  // Compute polar coordinates.
	  const float R = sqrt(x*x + y*y);
	  if (R > (z*z)) {
	    continue;
	  }
	  const float Theta = atan2(y, x);
	  
	  // Compute spherical coordinates.
	  const float phi = 2.0f * z * atan2(1.0f, R);
	  const float theta = Theta;
	  // Compute lookup in original image.
	  int image_i = static_cast<int>(theta * M_1_PI * 0.5f * width);
	  int image_j = static_cast<int>(phi * 2.0f * M_1_PI * height);
	  if (image_i < 0) {
	    image_i += width;
	  } else if (image_i >= width) {
	    image_i -= width;
	  }
	  if (image_j < 0) {
	    image_j += height;
	  } else if (image_j >= height) {
	    image_j -= height * floor(image_j/height);
	  }
	  // Copy pixel.
	  for (int c = 0; c < image.spectrum(); c++) {
	    (*rectified_image)(i,j,0,c) = image(image_i, image_j, 0, c);
	  }
	}
      }
    }
    
    void CylindricalEqualAreaProjection(const FloatImage& image, 
					FloatImage* rectified_image) {
      const float height = image.height();
      
      const float minPhi = -M_PI / 2.0f;// + M_PI / 2.2f;
      const float maxPhi = M_PI / 2.0f;// - M_PI / 2.2f;
      
      cimg_forXY(image, i, j) {
	const float phi 
	  = minPhi + (maxPhi - minPhi) * static_cast<float>(j) / height;
	
	const float x = static_cast<float>(i);
	const float sinPhi = sin(phi);
	const float y = (sinPhi + 1.0f) / 2.0f;
	
	const int32 outputx = static_cast<int32>(x);
	const int32 outputy = static_cast<int32>(y * height);
#if 0
	ASSERT_GTE(outputx, 0);
	ASSERT_LT(outputx, image.width());
	ASSERT_GTE(outputy, 0);
	ASSERT_LTE(outputy, image.height());
#endif
	cimg_forC(image, c) {
	  //rectified_image->set_linear_atXY(image(i, j, 0, c), outputx, outputy, 0, c);
	  //(*rectified_image)(outputx, outputy, 0, c) = image(i, j, 0, c);
	  (*rectified_image)(i, j, 0, c) = image(outputx, outputy, 0, c);
	}
      }
    }
    
    void InverseStereographicProjection(const FloatImage& image, FloatImage* rectified_image, 
					const float& pitch, const float& heading) {
      const float width = rectified_image->width();
      const float height = rectified_image->height();
      const float iwidth = image.width();
      const float iheight = image.height();
      
      const float R = 1.0f;
      
      const float phic = pitch;
      const float lamc = heading;
      
      cimg_forXY(*rectified_image, ix, iy) {
	const float x = 2.0f * R * static_cast<float>(ix) / iwidth - R;
	const float y = 2.0f * R * static_cast<float>(iy) / iheight - R;
	
	if (x == 0.0f && y == 0.0f) {
	  continue;
	}
	
	const float p = sqrt(x*x + y*y);
	const float c = 2.0f * atan2(p, 2.0f * R);
	
	const float phi = asin(cos(c) * sin(phic) 
			       + (y * sin(c) * cos(phic)) / p);  // [-pi/2, pi/2]
	const float lam 
	  = lamc + 
	  atan2((x * sin(c)), 
		(p * cos(phic) * cos(c) - y * sin(phic) * sin(c)));  // [-pi/2, pi/2]
	float lookupx = iwidth *  (lam + M_PI / 1.0f) / (M_PI / 0.5f);
	float lookupy = iheight * (phi + M_PI / 2.0f) / (M_PI / 1.0f);
	
	lookupx = lookupx - floor(lookupx / iwidth) * iwidth;
	lookupy = lookupy - floor(lookupy / iheight) * iheight;
	
	/*
	  ASSERT_GTE(lookupx, 0);
	  ASSERT_LT(lookupx, width);
	  ASSERT_GTE(lookupy, 0);
	  ASSERT_LTE(lookupy, height);
	*/
	
	cimg_forC(*rectified_image, s) {
	  (*rectified_image)(ix, iy, 0, s) 
	    = image.linear_atXYZC(lookupx, lookupy, 0, s, 0.0f);
	}
      }
    }
    
  }
}
