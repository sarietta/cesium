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
    static void PrettyPrintFloatImage(const FloatImage& image);

    static void DisplayEigenMatrix(const FloatMatrix& matrix);

    static FloatImage FastFilter(const FloatImage& image, const FloatImage& kernel);

    static void DrawThickLine(const int x1, const int& y1, const int& x2, const int& y2,
			      const float& thickness, const float* color, const float& opacity,
			      FloatImage* image);

#ifndef SKIP_OPENCV
    static IplImage* GetIplImage(const UInt8Image& image) {
      const int32 width = image.width();
      const int32 height = image.height();
      const int32 channels = image.spectrum();
      
      IplImage* iplimage = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, channels);
      uint8* pels = (uint8*) iplimage->imageData;
      cimg_forXYC(image, x, y, c) {
	pels[y*width*channels + x*channels + c] = image(x, y, c);
      }
      return iplimage;
    }

    static UInt8Image GetCImg(const IplImage& image) {
      const int32 width = image.width;
      const int32 height = image.height;
      const int32 channels = image.nChannels;
      
      UInt8Image cimage(width, height, 1, channels);
      IplImage* ipluint8 = cvCreateImage(cvGetSize(&image), IPL_DEPTH_8U, channels);
      cvConvert(&image, ipluint8);
      
      uint8* pels = (uint8*) ipluint8->imageData;
      cimg_forXYC(cimage, x, y, c) {
	cimage(x, y, 0, c) = pels[y*image.widthStep + x*channels + (channels - 1 - c)];
      }
      cvReleaseImage(&ipluint8);
      return cimage;
    }
#endif

    static unsigned char* GetRowMajorByteArray(const UInt8Image& image) {
      const int32 width = image.width();
      const int32 height = image.height();
      const int32 channels = image.spectrum();
      
      unsigned char* data = new unsigned char[width * height * channels];
      cimg_forXYC(image, x, y, c) {
	data[c + x*channels + y*channels*width] = image(x,y,c);
      }
      
      return data;
    }
  };
}  // namespace slib
#endif
