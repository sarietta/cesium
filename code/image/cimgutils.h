#ifndef __SLIB_IMAGE_CIMGUTILS_H__
#define __SLIB_IMAGE_CIMGUTILS_H__

#define SLIB_NO_DEFINE_64BIT

#include <CImg.h>
#ifndef SKIP_OPENCV
#include <cv.h>
#endif
#undef Success
#include <Eigen/Dense>
#include <common/types.h>
#ifndef SKIP_OPENCV
#include <highgui.h>
#endif

using namespace cimg_library;

namespace slib {
  class CImgUtils {
  public:
#ifndef SKIP_OPENCV
    static IplImage* GetIplImage(const UInt8Image& image);
    static UInt8Image GetCImg(const IplImage& image);

    static unsigned char* GetRowMajorByteArray(const UInt8Image& image);
#endif
    static void PrettyPrintFloatImage(const FloatImage& image);

    static void DisplayEigenMatrix(const FloatMatrix& matrix);
  };
}  // namespace slib
#endif
