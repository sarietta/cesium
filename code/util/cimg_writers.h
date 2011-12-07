#ifndef __SLIB_UTIL_CIMG_WRITERS_H__
#define __SLIB_UTIL_CIMG_WRITERS_H__

#include <common/types.h>
#include <glog/logging.h>
#include <stdio.h>
#include <string>

using std::string;

namespace slib {
  namespace util {
#ifdef cimg_version
    template<typename T>
    bool SaveCImgToBinary(const cimg_library::CImg<T> image, const string& path) {
      FILE* file = fopen(path.c_str(), "wb");
      if (file == NULL) {
	LOG(WARNING) << "Could not open file for writing: " << path;
	return false;
      }

      const int w = image.width();
      const int h = image.height();
      const int d = image.depth();
      const int c = image.spectrum();
      
      int32 cnt = 0;
      cnt += fwrite(&w, sizeof(int), 1, file);
      cnt += fwrite(&h, sizeof(int), 1, file);
      cnt += fwrite(&d, sizeof(int), 1, file);
      cnt += fwrite(&c, sizeof(int), 1, file);
      if (cnt != 4) {
	LOG(WARNING) << "Could not save image to file: " << path;
	return false;
      }

      cnt = fwrite(image.data(), sizeof(T), w * h * d * c, file);
      if (cnt != w * h * d * c) {
	LOG(WARNING) << "Could not save image to file: " << path;
	return false;
      }

      fclose(file);
      return true;
    }
#endif
  }  // namespace util
}  // namespace slib
#endif
