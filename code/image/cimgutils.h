#ifndef __SLIB_IMAGE_CIMGUTILS_H__
#define __SLIB_IMAGE_CIMGUTILS_H__

#define SLIB_NO_DEFINE_64BIT

#include <CImg.h>
#include <common/types.h>
#include <cv.h>
#include <highgui.h>

using namespace cimg_library;

namespace slib {
  class CImgUtils {
  public:
    static IplImage* GetIplImage(const UInt8Image& image);
    static UInt8Image GetCImg(const IplImage& image);

    static unsigned char* GetRowMajorByteArray(const UInt8Image& image);
  };
}  // namespace slib
#endif
