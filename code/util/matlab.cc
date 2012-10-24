#include "matlab.h"

#include <common/types.h>
#include <glog/logging.h>
#include <mat.h>
#include <string>

using slib::svm::Model;
using std::string;

namespace slib {
  namespace util {
    
    MatlabMatrix::MatlabMatrix(const MatlabMatrixType& type) 
      : _matrix(NULL)
      , _type(type) {}

    MatlabMatrix::~MatlabMatrix() {
      if (_matrix != NULL) {
	mxDestroyArray(_matrix);
      }
      _matrix = NULL;
    }

    MatlabMatrix::MatlabMatrix(const mxArray* data)
      : _matrix(NULL)
      , _type(MATLAB_NO_TYPE) {
      _type = GetType(data);
      _matrix = mxDuplicateArray(data);
    }

    MatlabMatrix::MatlabMatrix(const MatlabMatrix& matrix) 
      : _matrix(NULL)
      , _type(matrix._type) {
      if (matrix._matrix != NULL) {
	_matrix = mxDuplicateArray(matrix._matrix);
      }
    }

    MatlabMatrix::MatlabMatrix(const string& filename) 
      : _matrix(NULL)
      , _type(MATLAB_NO_TYPE) {
      LoadMatrixFromFile(filename);
    }
    
    MatlabMatrix::MatlabMatrix(const FloatMatrix& contents) 
      : _matrix(NULL)
      , _type(MATLAB_MATRIX) {
      SetContents(contents);
    }

    MatlabMatrixType MatlabMatrix::GetType(const mxArray* data) {
      if (mxIsNumeric(data)) {
	return MATLAB_MATRIX;
      } else if (mxIsStruct(data)) {
	return MATLAB_STRUCT;
      } else if (mxIsCell(data)) {
	return MATLAB_CELL_ARRAY;
      } else {
	LOG(WARNING) << "Unknown Matlab matrix type";
	return MATLAB_NO_TYPE;
      }
    }
    
    MatlabMatrix MatlabMatrix::LoadFromFile(const string& filename) {
      MatlabMatrix matrix(filename);
      return matrix;
    }
    
    MatlabMatrix MatlabMatrix::GetStructField(const string& field, const int& index) const {
      if (_type == MATLAB_STRUCT) {
	// Check to make sure the field exists.
	if (mxGetFieldNumber(_matrix, field.c_str()) == -1) {
	  LOG(WARNING) << "No field named: " << field;
	  return MatlabMatrix(MATLAB_NO_TYPE);
	}

	const mxArray* data = mxGetField(_matrix, index, field.c_str());
	return MatlabMatrix(data);
      } else {
	LOG(WARNING) << "Attempted to access non-struct (field: " << field << ")";
	return MatlabMatrix(MATLAB_NO_TYPE);
      }
    }

    MatlabMatrix MatlabMatrix::GetCell(const int& row, const int& col) const {
      if (_type == MATLAB_CELL_ARRAY) {
	const mwIndex subscripts[2] = {row, col};
	const int index = mxCalcSingleSubscript(_matrix, 2, subscripts);
	const mxArray* data = mxGetCell(_matrix, index);
	return MatlabMatrix(data);
      } else {
	LOG(WARNING) << "Attempted to access non-cell array (" << row << ", " << col << ")";
	return MatlabMatrix(MATLAB_NO_TYPE);
      }
    }

    FloatMatrix MatlabMatrix::GetContents() const {
      FloatMatrix matrix;
      if (_type == MATLAB_MATRIX) {
	const int dimensions = mxGetNumberOfDimensions(_matrix);
	if (dimensions > 2) {
	  LOG(ERROR) << "Only 2D matrices are supported";
	  return matrix;
	}
	const int rows = mxGetM(_matrix);
	const int cols = mxGetN(_matrix);
	matrix.resize(rows, cols);
	matrix.fill(0.0f);

	if (mxIsDouble(_matrix)) {
	  const double* data = (double*) mxGetData(_matrix);
	  for (int i = 0; i < cols; i++) {
	    for (int j = 0; j < rows; j++) {
	      matrix(j, i) = (float) data[j + i * rows];
	    }
	  }
	} else if (mxIsSingle(_matrix)) {
	  const float* data = (float*) mxGetData(_matrix);
	  for (int i = 0; i < cols; i++) {
	    for (int j = 0; j < rows; j++) {
	      matrix(j, i) = data[j + i * rows];
	    }
	  }
	} else {
	  LOG(ERROR) << "Only float and double matrices are supported";
	}
      } else {
	LOG(WARNING) << "Attempted to access non-matrix";
      }

      return matrix;
    }
    
    void MatlabMatrix::SetStructField(const string& field, const MatlabMatrix& contents) {
      SetStructField(field, 0, contents);
    }

    void MatlabMatrix::SetStructField(const string& field, 
				      const int& index, const MatlabMatrix& contents) {
      if (_type == MATLAB_STRUCT) {
	// Check to see if the field already exists.
	if (mxGetFieldNumber(_matrix, field.c_str()) != -1) {
	  // If it exists, have to destroy it.
	  mxArray* data = mxGetField(_matrix, index, field.c_str());
	  mxDestroyArray(data);
	}

	// Deep copy.
	mxArray* data = mxDuplicateArray(contents._matrix);
	mxSetField(_matrix, index, field.c_str(), data);
      } else {
	LOG(WARNING) << "Attempted to access non-struct (field: " << field << ")";
      }
    }

    void MatlabMatrix::SetCell(const int& row, const int& col, const MatlabMatrix& contents) {
      if (_type == MATLAB_CELL_ARRAY) {
	const mwIndex subscripts[2] = {row, col};
	const int index = mxCalcSingleSubscript(_matrix, 2, subscripts);

	// Check to see if the cell has already been set.
	if (mxGetCell(_matrix, index) != NULL) {
	  // If it exists, we have to destroy it.
	  mxArray* data = mxGetCell(_matrix, index);
	  mxDestroyArray(data);
	}

	// Deep copy.
	mxArray* data = mxDuplicateArray(contents._matrix);
	mxSetCell(_matrix, index, data);
      } else {
	LOG(WARNING) << "Attempted to access non-cell array (" << row << ", " << col << ")";
      }
    }

    void MatlabMatrix::SetContents(const FloatMatrix& contents) {
      if (_type == MATLAB_MATRIX) {
	// Overwrite the already existing data if necessary.
	if (_matrix != NULL) {
	  mxDestroyArray(_matrix);
	}
	// Matlab matrices are in column-major order.
	_matrix = mxCreateNumericMatrix(contents.rows(), contents.cols(), mxSINGLE_CLASS, mxREAL);
	float* data = (float*) mxGetData(_matrix);
	for (int i = 0; i < contents.cols(); i++) {
	  for (int j = 0; j < contents.rows(); j++) {
	    data[j + i * contents.rows()] = contents(j, i);
	  }
	}
      } else {
	LOG(WARNING) << "Attempted to access non-matrix";
      }
    }
        
    void MatlabMatrix::LoadMatrixFromFile(const string& filename) {
      MATFile* pmat = matOpen(filename.c_str(), "r");
      if (pmat == NULL) {
	LOG(ERROR) << "Error Opening MAT File: " << filename;
	return;
      }
      VLOG(1) << "Reading matrix from file: " << filename;

      // Should only have one entry.
      const char* name = NULL;
      mxArray* data = matGetNextVariable(pmat, &name);
      VLOG(1) << "Found variable " << name << " in file: " << filename;
      if (matGetNextVariable(pmat, &name) != NULL) {
	LOG(WARNING) << "Only one entry per MAT-file supported (name: " << name << ")";
      }

      _type = GetType(data);
      _matrix = mxDuplicateArray(data);

      mxDestroyArray(data);
      matClose(pmat);
    }

    bool MatlabMatrix::SaveToFile(const string& filename, const string& variable_name) const {
      MATFile* pmat = matOpen(filename.c_str(), "w");
      if (pmat == NULL) {
	LOG(ERROR) << "Error Opening MAT File: " << filename;
	return false;
      }
      VLOG(1) << "Writing matrix to file: " << filename;

      if (_matrix == NULL || matPutVariable(pmat, variable_name.c_str(), _matrix) != 0) {
	LOG(ERROR) << "Error writing matrix data to file: " << filename;
	matClose(pmat);
	return false;
      }

      matClose(pmat);

      return true;
    }

    MatlabMatrix MatlabConverter::ConvertModelToMatix(const Model& model) {
      return MatlabMatrix(MATLAB_NO_TYPE);
    }
  }  // namespace util
}  // namespace slib
  
