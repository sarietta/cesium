#define SLIB_NO_DEFINE_64BIT

#include <CImg.h>
#include <common/types.h>
#ifndef SKIP_OPENCV
#include <cv.h>
#include <highgui.h>
#endif

#include "cimgutils.h"

using namespace cimg_library;

namespace slib {

#ifndef SKIP_OPENCV
  IplImage* CImgUtils::GetIplImage(const UInt8Image& image) {
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

  UInt8Image CImgUtils::GetCImg(const IplImage& image) {
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

  unsigned char* CImgUtils::GetRowMajorByteArray(const UInt8Image& image) {
    const int32 width = image.width();
    const int32 height = image.height();
    const int32 channels = image.spectrum();

    unsigned char* data = new unsigned char[width * height * channels];
    cimg_forXYC(image, x, y, c) {
      data[c + x*channels + y*channels*width] = image(x,y,c);
    }

    return data;
  }
#endif

  void CImgUtils::PrettyPrintFloatImage(const FloatImage& image) {
    if (image.size() > 100) {
      fprintf(stderr, "Not outputting images larger than 100 values total.\n");
      return;
    }
    cimg_forC(image, c) {
      printf("image(:,:,%d) = \n", c);
      cimg_forY(image, y) {
	printf("\n\t");
	cimg_forX(image, x) {
	  printf("%4.4f\t", image(x,y,c));
	}
      }
      printf("\n\n");
    }
    printf("\n\n");
  }

  void CImgUtils::DisplayEigenMatrix(const FloatMatrix& matrix) {
    UInt8Image image(matrix.cols(), matrix.rows(), 1, 1);
    for (int row = 0; row < matrix.rows(); row++) {
      for (int col = 0; col < matrix.cols(); col++) {
	image(col, row) = (char) (matrix(row, col) * 255.0f);
      }
    }

    image.map(CImg<unsigned char>::jet_LUT256());
    image.display();
  }

}  // namespace slib
