#include "eigenutils.h"

#include <glog/logging.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>

using std::string;

namespace slib {
  namespace util {
    bool EigenUtils::SaveMatrixToFile(const string& filename, const FloatMatrix& matrix) {
      FILE* fid = fopen(filename.c_str(), "wb");
      if (!fid) {
	LOG(ERROR) << "Could not open file for writing: " << filename;
	return false;
      }

      const int rows = matrix.rows();
      const int cols = matrix.cols();

      int count = 0;
      count += fwrite(&rows, sizeof(int), 1, fid);
      count += fwrite(&cols, sizeof(int), 1, fid);
      if (count != 2) {
	LOG(ERROR) << "Could not write matrix to file: " << filename;
	fclose(fid);
	return false;
      }

      count = 0;
      count += fwrite(matrix.data(), sizeof(float), rows * cols, fid);
      if (count != rows * cols) {
	LOG(ERROR) << "Could not write matrix to file: " << filename;
	fclose(fid);
	return false;
      }

      fclose(fid);
      return true;
    }
  }  // namespace util
}  // namespace slib
