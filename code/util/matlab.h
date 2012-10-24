#ifndef __SLIB_UTIL_MATLAB_H__
#define __SLIB_UTIL_MATLAB_H__

#include <common/scoped_ptr.h>
#include <common/types.h>
#undef Success
#include <Eigen/Dense>
#include <map>
#include <mat.h>
#include <string>
#include <svm/model.h>

namespace slib {
  namespace util {

    enum MatlabMatrixType {
      MATLAB_STRUCT, MATLAB_CELL_ARRAY, MATLAB_MATRIX, MATLAB_NO_TYPE
    };

    class MatlabMatrix {
    public:
      explicit MatlabMatrix(const MatlabMatrixType& type);
      virtual ~MatlabMatrix();
      MatlabMatrix(const MatlabMatrix& matrix);
      explicit MatlabMatrix(const std::string& filename);

      explicit MatlabMatrix(const FloatMatrix& contents);

      static MatlabMatrix LoadFromFile(const std::string& filename);
      bool SaveToFile(const std::string& filename, const std::string& variable_name = "data") const;

      MatlabMatrix GetStructField(const std::string& field, const int& index = 0) const;
      MatlabMatrix GetCell(const int& row, const int& col) const;
      FloatMatrix GetContents() const;

      void SetStructField(const std::string& field, const MatlabMatrix& contents);
      void SetStructField(const std::string& field, const int& index, const MatlabMatrix& contents);
      void SetCell(const int& row, const int& col, const MatlabMatrix& contents);
      void SetContents(const FloatMatrix& contents);

    private:
      mxArray* _matrix;
      MatlabMatrixType _type;

      MatlabMatrix(const mxArray* data);

      void LoadMatrixFromFile(const std::string& filename);
      MatlabMatrixType GetType(const mxArray* data);
    };

    class MatlabConverter {
    public:
      static MatlabMatrix ConvertModelToMatix(const slib::svm::Model& model);
    };

  }  // namespace util
}  // namespace slib

#endif
