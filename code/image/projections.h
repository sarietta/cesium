#ifndef __SLIB_IMAGE_PROJECTIONS_H__
#define __SLIB_IMAGE_PROJECTIONS_H__

#include <CImg.h>
#include <common/types.h>

namespace slib {
  namespace image {

    void PerspectiveProjection(const FloatImage& image, FloatImage* rectified_image, 
			       const float& fov, const float& heading, const float& pitch,
			       const bool& use_cubic_interpolation = false);

    // These are semi-untested. Most of them should be close to correct.
    void StereographicProjection(const FloatImage& image, FloatImage* rectified_image, const float& z);
    void CylindricalEqualAreaProjection(const FloatImage& image, FloatImage* rectified_image);
    void InverseStereographicProjection(const FloatImage& image, FloatImage* rectified_image,
					const float& pitch, const float& heading);
  }
}

#endif  // __SLIB_IMAGE_PROJECTIONS_H__
