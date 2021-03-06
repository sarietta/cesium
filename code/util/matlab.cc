#include "matlab.h"

#include "assert.h"
#include <CImg.h>
#include <common/types.h>
#include <glog/logging.h>
#include <iostream>
#include <mat.h>
#include <string>
#include <sstream>
#include <svm/detector.h>
#include <vector>
#include <wchar.h>

using slib::svm::DetectionMetadata;
using slib::svm::DetectionResultSet;
using slib::svm::DetectorFactory;
using slib::svm::Detector;
using slib::svm::Model;
using std::string;
using std::stringstream;
using std::vector;

namespace slib {
  namespace util {

    MatlabMatrix::MatlabMatrix() 
      : _matrix(NULL)
      , _shared(false)
      , _type(MATLAB_NO_TYPE) {}
    
    MatlabMatrix::MatlabMatrix(const MatlabMatrixType& type) 
      : _matrix(NULL)
      , _shared(false)
      , _type(type) {}

    MatlabMatrix::MatlabMatrix(const MatlabMatrixType& type, const Pair<int>& dimensions) 
      : _matrix(NULL)
      , _shared(false)
      , _type(type) {
      Initialize(type, dimensions);
    }

    MatlabMatrix::MatlabMatrix(const MatlabMatrixType& type, const int& rows, const int& cols) 
      : _matrix(NULL)
      , _shared(false)
      , _type(type) {
      Initialize(type, Pair<int>(rows, cols));
    }

    MatlabMatrix::MatlabMatrix(const std::vector<std::string>& values, const bool& col) 
      : _matrix(NULL), _type(MATLAB_CELL_ARRAY) {
      if (col) {
	Initialize(_type, Pair<int>(values.size(), 1));
	for (int i = 0; i < values.size(); i++) {
	  SetCell(i, 0, MatlabMatrix(values[i]));
	}
      } else {
	Initialize(_type, Pair<int>(1, values.size()));
	for (int i = 0; i < values.size(); i++) {
	  SetCell(0, i, MatlabMatrix(values[i]));
	}
      }
    }

    void MatlabMatrix::Initialize(const MatlabMatrixType& type, const Pair<int>& dimensions) {
      _shared = false;
      _type = type;
      if (_type == MATLAB_STRUCT) {
	_matrix = mxCreateStructMatrix(dimensions.x, dimensions.y, 0, NULL);
      } else if (_type == MATLAB_CELL_ARRAY) {
	_matrix = mxCreateCellMatrix(dimensions.x, dimensions.y);
      } else if (_type == MATLAB_MATRIX) {
	_matrix = mxCreateNumericMatrix(dimensions.x, dimensions.y, mxSINGLE_CLASS, mxREAL);
      } else if (_type == MATLAB_STRING) {
	const int length = dimensions.x > dimensions.y ? dimensions.x : dimensions.y;
	const string str(length, '\0');
	_matrix = mxCreateString(str.c_str());
      } else if (_type == MATLAB_MATRIX_SPARSE) {
	_matrix = mxCreateSparse(dimensions.x, dimensions.y, 0, mxREAL);
      }
    }

    MatlabMatrix::~MatlabMatrix() {
      if (_matrix != NULL && !_shared) {
	mxDestroyArray(_matrix);
      }
      _matrix = NULL;
    }

    MatlabMatrix::MatlabMatrix(const mxArray* data)
      : _matrix(NULL)
      , _shared(false)
      , _type(MATLAB_NO_TYPE) {
      if (data != NULL) {
	_type = GetType(data);
	_matrix = mxDuplicateArray(data);
      }
    }

    MatlabMatrix::MatlabMatrix(mxArray* data)
      : _matrix(NULL)
      , _shared(true)
      , _type(MATLAB_NO_TYPE) {
      if (data != NULL) {
	_type = GetType(data);
	// WARNING: This is NOT a deep copy. Shared pointer!
	_matrix = data;
      }
    }

    MatlabMatrix::MatlabMatrix(const MatlabMatrix& matrix) 
      : _matrix(NULL)
      , _shared(false)
      , _type(matrix._type) {
      if (matrix._matrix != NULL) {
	_matrix = mxDuplicateArray(matrix._matrix);
      }
    }

    MatlabMatrix::MatlabMatrix(const string& contents) 
      : _matrix(NULL)
      , _shared(false)
      , _type(MATLAB_STRING) {
      SetStringContents(contents);
    }
    
    MatlabMatrix::MatlabMatrix(const float& value)
      : _matrix(NULL)
      , _shared(false)
      , _type(MATLAB_MATRIX) {
      FloatMatrix contents(1,1);
      contents(0,0) = value;
      SetContents(contents);
    }
    
    MatlabMatrix::MatlabMatrix(const float* contents, const int& rows, const int& cols) 
      : _matrix(NULL)
      , _shared(false)
      , _type(MATLAB_MATRIX) {
      FloatMatrix matrix(rows, cols);
      memcpy(matrix.data(), contents, sizeof(float) * rows * cols);
      SetContents(matrix);
    }

    MatlabMatrix::MatlabMatrix(const FloatMatrix& contents) 
      : _matrix(NULL)
      , _shared(false)
      , _type(MATLAB_MATRIX) {
      SetContents(contents);
    }

    void MatlabMatrix::AssignData(mxArray* data) {
      if (data != NULL) {
	_type = GetType(data);
	_shared = true;
	// WARNING: This is NOT a deep copy. Shared pointer!
	_matrix = data;
      }
    }

    const MatlabMatrix& MatlabMatrix::operator=(const MatlabMatrix& right) {
      Assign(right);
      return (*this);
    }

    void MatlabMatrix::Assign(const MatlabMatrix& other) {
      if (_matrix != NULL) {
	mxDestroyArray(_matrix);
	_matrix = NULL;
      }
      if (other._matrix != NULL) {
	_matrix = mxDuplicateArray(other._matrix);
      } else {
	_matrix = NULL;
      }
      _type = other._type;
      _shared = false;
    }

    void MatlabMatrix::Assign(const FloatMatrix& other) {
      _type = MATLAB_MATRIX;
      _shared = false;
      SetContents(other);
    }

    MatlabMatrix& MatlabMatrix::Merge(const MatlabMatrix& other) {
      if (_type == MATLAB_NO_TYPE && _matrix == NULL) {
	Initialize(other._type, other.GetDimensions());
      }

      if (_type != other._type) {
	return *this;
      }      

      // Resize if necessary
      Pair<int> dimensions = GetDimensions();
      const Pair<int> other_dimensions = other.GetDimensions();
      if (dimensions.x < other_dimensions.x || dimensions.y < other_dimensions.y) {
	const int rows = dimensions.x > other_dimensions.x ? dimensions.x : other_dimensions.x;
	const int cols = dimensions.y > other_dimensions.y ? dimensions.y : other_dimensions.y;

	MatlabMatrix copy(*this);
	mxDestroyArray(_matrix);
	Initialize(other._type, Pair<int>(rows, cols));
	Merge(copy);
	Merge(other);
	return *this;
      }

      const int num_elements = other_dimensions.x * other_dimensions.y;

      switch(_type) {
      case MATLAB_STRUCT: {
	vector<string> other_fields = other.GetStructFieldNames();

	for (int i = 0; i < num_elements;  i++) {
	  for (uint32 j = 0; j < other_fields.size(); j++) {
	    const MatlabMatrix this_field = GetCopiedStructField(other_fields[j], i);
	    if (this_field.GetMatrixType() == MATLAB_NO_TYPE) {
	      SetStructField(other_fields[j], i, other.GetCopiedStructField(other_fields[j], i));
	    }
	  }
	}

	break;
      } 
      case MATLAB_CELL_ARRAY:
	for (int i = 0; i < num_elements; i++) {
	  const MatlabMatrix this_cell = GetCopiedCell(i);
	  if (this_cell.GetMatrixType() == MATLAB_NO_TYPE) {	
	    SetCell(i, other.GetCopiedCell(i));
	  }
	}
	break;
      case MATLAB_MATRIX:
      case MATLAB_STRING:
      default:
	LOG(WARNING) << "Cannot merge matrices... must be struct or cell array";
	break;
      }

      return *this;
    }

    MatlabMatrixType MatlabMatrix::GetType(const mxArray* data) const {
      if (data == NULL) {
	return MATLAB_NO_TYPE;
      }
      if (mxIsSparse(data)) {
	return MATLAB_MATRIX_SPARSE;
      } else if (mxIsNumeric(data)) {
	return MATLAB_MATRIX;
      } else if (mxIsStruct(data)) {
	return MATLAB_STRUCT;
      } else if (mxIsCell(data)) {
	return MATLAB_CELL_ARRAY;
      } else if (mxIsChar(data)) {
	return MATLAB_STRING;
      } else {
	return MATLAB_NO_TYPE;
      }
    }
    
    Pair<int> MatlabMatrix::GetDimensions() const {
      if (_matrix == NULL) {
	return Pair<int>(0, 0);
      }

      const int num_dimensions = mxGetNumberOfDimensions(_matrix);
      if (num_dimensions > 2) {
	LOG(ERROR) << "There is a matrix in here that has more than 2 dimensions!";
      }
      const mwSize* dimensions = mxGetDimensions(_matrix);
      if (num_dimensions == 2) {
	return Pair<int>(dimensions[0], dimensions[1]);
      } else {
	return Pair<int>(dimensions[0], 1);
      }
    }

    vector<string> MatlabMatrix::GetStructFieldNames() const {
      const int num_fields = mxGetNumberOfFields(_matrix);
      vector<string> fields;
      for (int i = 0; i < num_fields; i++) {
	string field(mxGetFieldNameByNumber(_matrix, i));
	fields.push_back(field);
      }
      return fields;
    }
    
    MatlabMatrix MatlabMatrix::LoadFromFile(const string& filename, const bool& multivariable) {
      MatlabMatrix matrix;
      matrix.LoadMatrixFromFile(filename, multivariable);
      return matrix;
    }

    MatlabMatrix MatlabMatrix::LoadFromBinaryFile(const string& filename) {
      FILE* fid = fopen(filename.c_str(), "rb");
      int length;
      fread(&length, sizeof(int), 1, fid);
      char buffer[4096];
      string contents;
      while (!feof(fid)) {
	const int read = fread(buffer, sizeof(char), 4096, fid);
	contents.append(buffer, read);
      }
      fclose(fid);
      MatlabMatrix matrix;
      matrix.Deserialize(contents);
      return matrix;
    }

    float MatlabMatrix::GetMatrixEntry(const int& row, const int& col) const {
      const mwIndex subscripts[2] = {(mwIndex) row, (mwIndex) col};
      const int index = mxCalcSingleSubscript(_matrix, 2, subscripts);
      return GetMatrixEntry(index);
    }

    float MatlabMatrix::GetMatrixEntry(const int& index) const {
      if (_matrix != NULL && _type == MATLAB_MATRIX) {
	if (mxIsDouble(_matrix)) {
	  const double* data = (double*) mxGetData(_matrix);
	  return ((float) data[index]);
	} else if (mxIsSingle(_matrix)) {
	  const float* data = (float*) mxGetData(_matrix);
	  return data[index];
	} else {
	  LOG(ERROR) << "Only float and double matrices are supported";
	}
      } else {
	VLOG(2) << "Attempted to access non-matrix";
      }

      return 0.0f;
    }

    bool MatlabMatrix::HasStructField(const string& field, const int& row, const int& col) const {
      const mwIndex subscripts[2] = {(mwIndex) row, (mwIndex) col};
      const int index = mxCalcSingleSubscript(_matrix, 2, subscripts);
      return HasStructField(field, index);
    }

    bool MatlabMatrix::HasStructField(const string& field, const int& index) const {
      if (_matrix != NULL && _type == MATLAB_STRUCT) {
	if (mxGetFieldNumber(_matrix, field.c_str()) == -1) {
	  return false;
	} else {
	  return true;
	}
      } else {
	return false;
      }
    }

    const MatlabMatrix MatlabMatrix::GetStructField(const string& field, const int& row, const int& col) const {
      const mwIndex subscripts[2] = {(mwIndex) row, (mwIndex) col};
      const int index = mxCalcSingleSubscript(_matrix, 2, subscripts);
      return GetStructField(field, index);
    }

    const MatlabMatrix MatlabMatrix::GetStructField(const string& field, const int& index) const {
      if (_matrix != NULL && _type == MATLAB_STRUCT) {
	// Check to make sure the field exists.
	if (mxGetFieldNumber(_matrix, field.c_str()) == -1) {
	  LOG(WARNING) << "No field named: " << field;
	  return MatlabMatrix(MATLAB_NO_TYPE);
	}

	mxArray* data = mxGetField(_matrix, index, field.c_str());
	return MatlabMatrix(data);
      } else {
	VLOG(2) << "Attempted to access non-struct (field: " << field << ")";
	return MatlabMatrix(MATLAB_NO_TYPE);
      }
    }
    
    const MatlabMatrix MatlabMatrix::GetCell(const int& row, const int& col) const {
      const mwIndex subscripts[2] = {(mwIndex) row, (mwIndex) col};
      const int index = mxCalcSingleSubscript(_matrix, 2, subscripts);
      return GetCell(index);
    }

    const MatlabMatrix MatlabMatrix::GetCell(const int& index) const {
      if (_matrix != NULL && _type == MATLAB_CELL_ARRAY) {
	mxArray* data = mxGetCell(_matrix, index);
	return MatlabMatrix(data);
      } else {
	VLOG(2) << "Attempted to access non-cell array (" << index << ")";
	return MatlabMatrix(MATLAB_NO_TYPE);
      }
    }

    void MatlabMatrix::GetMutableCell(const int& row, const int& col, MatlabMatrix* cell) const {
      const mwIndex subscripts[2] = {(mwIndex) row, (mwIndex) col};
      const int index = mxCalcSingleSubscript(_matrix, 2, subscripts);
      GetMutableCell(index, cell);
    }

    void MatlabMatrix::GetMutableCell(const int& index, MatlabMatrix* cell) const {
      if (_matrix != NULL && _type == MATLAB_CELL_ARRAY) {
	mxArray* data = mxGetCell(_matrix, index);
	cell->AssignData(data);
      } else {
	VLOG(2) << "Attempted to access non-cell array (" << index << ")";
      }
    }

    MatlabMatrix MatlabMatrix::GetCopiedStructField(const string& field, const int& row, const int& col) const {
      const mwIndex subscripts[2] = {(mwIndex) row, (mwIndex) col};
      const int index = mxCalcSingleSubscript(_matrix, 2, subscripts);
      return GetCopiedStructField(field, index);
    }
    
    MatlabMatrix MatlabMatrix::GetCopiedStructField(const string& field, const int& index) const {
      if (_matrix != NULL && _type == MATLAB_STRUCT) {
	// Check to make sure the field exists.
	if (mxGetFieldNumber(_matrix, field.c_str()) == -1) {
	  LOG(WARNING) << "No field named: " << field;
	  return MatlabMatrix(MATLAB_NO_TYPE);
	}

	const mxArray* data = mxGetField(_matrix, index, field.c_str());
	return MatlabMatrix(data);
      } else {
	VLOG(2) << "Attempted to access non-struct (field: " << field << ")";
	return MatlabMatrix(MATLAB_NO_TYPE);
      }
    }

    MatlabMatrix MatlabMatrix::GetCopiedCell(const int& row, const int& col) const {
      const mwIndex subscripts[2] = {(mwIndex) row, (mwIndex) col};
      const int index = mxCalcSingleSubscript(_matrix, 2, subscripts);
      return GetCopiedCell(index);
    }

    MatlabMatrix MatlabMatrix::GetCopiedCell(const int& index) const {
      if (_matrix != NULL && _type == MATLAB_CELL_ARRAY) {
	const mxArray* data = mxGetCell(_matrix, index);
	return MatlabMatrix(data);
      } else {
	VLOG(2) << "Attempted to access non-cell array (" << index << ")";
	return MatlabMatrix(MATLAB_NO_TYPE);
      }
    }

    float MatlabMatrix::GetScalar() const {
      if (_matrix != NULL && _type == MATLAB_MATRIX) {
	const int dimensions = mxGetNumberOfDimensions(_matrix);
	if (dimensions > 2) {
	  LOG(ERROR) << "Only 2D matrices are supported";
	  return 0.0f;
	}
	const int rows = mxGetM(_matrix);
	const int cols = mxGetN(_matrix);
	if (rows != 1 || cols != 1) {
	  LOG(ERROR) << "Attempted to access non-scalar matrix: " << rows << " x " << cols;
	  return 0.0f;
	}
#if 0
	float value = 0.0f;
	if (mxIsSingle(_matrix)) {
	  value = ((float*) mxGetData(_matrix))[0];
	} else if (mxIsDouble(_matrix)) {
	  value = (float) ((double*) mxGetData(_matrix))[0];
	} else {
	  LOG(ERROR) << "Attempted to access non-numeric matrix";
	}
	return value;
#else
	return ((float) mxGetScalar(_matrix));
#endif
      } else {
	VLOG(2) << "Attempted to access non-matrix";
      }
      return 0.0f;
    }

    MatlabMatrix& MatlabMatrix::SetScalar(const float& scalar) {
      if (_matrix != NULL && _type == MATLAB_MATRIX) {
	const int dimensions = mxGetNumberOfDimensions(_matrix);
	if (dimensions > 2) {
	  LOG(ERROR) << "Only 2D matrices are supported";
	  return (*this);
	}
	const int rows = mxGetM(_matrix);
	const int cols = mxGetN(_matrix);
	if (rows != 1 || cols != 1) {
	  LOG(ERROR) << "Attempted to access non-scalar matrix";
	  return (*this);
	}

	if (mxIsSingle(_matrix)) {
	  float* data = (float*) mxGetData(_matrix);
	  data[0] = scalar;
	} else {
	  LOG(ERROR) << "Only single precision matrices supported";
	}
      } else {
	VLOG(2) << "Attempted to access non-matrix";
      }
      return (*this);
    }

    MatlabMatrix MatlabMatrix::CellToMatrix() const {
      MatlabMatrix matrix;

      if (GetNumberOfElements() == 0) {
	return matrix;
      }

      if (GetMatrixType() != MATLAB_CELL_ARRAY) {
	LOG(ERROR) << "Attempted to convert a non-cell array.";
	return matrix;
      }

      // Determine the type of the cells.
      MatlabMatrixType type = MATLAB_NO_TYPE;
      for (int i = 0; i < GetNumberOfElements(); i++) {
	if (GetCell(i).GetMatrixType() != MATLAB_NO_TYPE && type == MATLAB_NO_TYPE) {
	  type = GetCell(i).GetMatrixType();
	} else if (GetCell(i).GetMatrixType() != MATLAB_NO_TYPE && type != MATLAB_NO_TYPE) {
	  // Make sure all of the non-empty cells are the same type.
	  if (GetCell(i).GetMatrixType() != type) {
	    LOG(ERROR) << "Cell arrays can only be converted to matrices if all cells are the same type: " << i;
	    return matrix;
	  }
	}
      }

      int total_elements = 0;
      int cols = 0;
      for (int i = 0; i < GetNumberOfElements(); i++) {
	const int M = GetCell(i).rows();
	total_elements += M;
	
	if (cols == 0 && GetCell(i).GetMatrixType() != MATLAB_NO_TYPE && GetCell(i).cols() != 0) {
	  cols = GetCell(i).cols();
	} else if (cols != 0 && 
		   GetCell(i).GetMatrixType() != MATLAB_NO_TYPE && 
		   GetCell(i).cols() != 0 && 
		   GetCell(i).cols() != cols) {
	  LOG(ERROR) << "All cell entries must have the same number of columns.";
	  return matrix;
	}
      }
      VLOG(2) << "Well-formed cell array: " << total_elements << "x" << cols;

      if (type == MATLAB_STRUCT) {
	matrix.Assign(MatlabMatrix(MATLAB_STRUCT, total_elements, 1));
  
	int offset = 0;
	for (int i = 0; i < GetNumberOfElements(); i++) {
	  const MatlabMatrix& cell = GetCell(i);
	  if (cell.GetMatrixType() == MATLAB_NO_TYPE || cell.cols() == 0) {
	    continue;
	  }

	  for (int j = 0; j < cell.GetNumberOfElements(); j++) {
	    matrix.Set(offset, 0, cell.Get(j, 0));
	    offset++;
	  }
	}
      } else if (type == MATLAB_MATRIX) {
	FloatMatrix output_data(total_elements, cols);

	int offset = 0;
	for (int i = 0; i < GetNumberOfElements(); i++) {
	  const MatlabMatrix& cell = GetCell(i);
	  if (cell.GetMatrixType() == MATLAB_NO_TYPE || cell.cols() == 0) {
	    continue;
	  }

	  const FloatMatrix data = cell.GetCopiedContents();
	  for (int j = 0; j < cell.rows(); j++) {
	    output_data.row(offset) = data.row(j);
	    offset++;
	  }
	}
	
	matrix.Assign(MatlabMatrix(output_data));
      } else {
	LOG(ERROR) << "Cannot flatten non-struct or non-matrix";
	return matrix;
      }

      return matrix;
    }
    
    // TODO(sean): Implement all conversions
    FloatMatrix MatlabMatrix::GetCopiedContents() const {
      FloatMatrix matrix;
      if (_matrix != NULL && _type == MATLAB_MATRIX) {
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
	} else if (mxIsUint8(_matrix)) {
	  const unsigned char* data = (unsigned char*) mxGetData(_matrix);
	  for (int i = 0; i < cols; i++) {
	    for (int j = 0; j < rows; j++) {
	      matrix(j, i) = data[j + i * rows];
	    }
	  }
	} else if (mxIsUint16(_matrix)) {
	  const unsigned short* data = (unsigned short*) mxGetData(_matrix);
	  for (int i = 0; i < cols; i++) {
	    for (int j = 0; j < rows; j++) {
	      matrix(j, i) = data[j + i * rows];
	    }
	  }
	} else {
	  LOG(ERROR) << "Only numeric matrices are supported (" << mxGetClassID(_matrix) << ")";
	}
      } else if (_matrix != NULL && _type == MATLAB_CELL_ARRAY) {
	matrix = CellToMatrix().GetCopiedContents();
      } else {
	VLOG(2) << "Attempted to access non-matrix";
      }

      return matrix;
    }

    SparseFloatMatrix MatlabMatrix::GetCopiedSparseContents() const {
      SparseFloatMatrix matrix;
      if (_matrix != NULL && _type == MATLAB_MATRIX_SPARSE) {
	const int dimensions = mxGetNumberOfDimensions(_matrix);
	if (dimensions > 2) {
	  LOG(ERROR) << "Only 2D matrices are supported";
	  return matrix;
	}
	const int rows = mxGetM(_matrix);
	const int cols = mxGetN(_matrix);
	matrix.resize(rows, cols);

	// All sparse matrices can be assumed to be double (I think).
	const mwSize nnz = mxGetNzmax(_matrix);
	VLOG(2) << "Number of non-zero sparse entries: " << nnz;
	const mwIndex* row_indices = mxGetIr(_matrix);
	const mwIndex* col_indices = mxGetJc(_matrix);
	const double* data = (double*) mxGetData(_matrix);

	matrix.reserve(nnz);
	vector<Eigen::Triplet<float> > entries(nnz);
	for (int col = 0; col < cols; col++) {
	  const int start_index = col_indices[col];
	  const int end_index = col_indices[col + 1];
	  for (int index = start_index; index < end_index; index++) {
	    const int row = row_indices[index];
	    const float value = data[index];

	    entries[index] = Eigen::Triplet<float>(row, col, value);
	  }
	}
	matrix.setFromTriplets(entries.begin(), entries.end());
      } else {
	VLOG(2) << "Attempted to access non-matrix";
      }

      return matrix;
    }

    const float* MatlabMatrix::GetContents() const {
      if (_matrix != NULL && _type == MATLAB_MATRIX) {
	const int dimensions = mxGetNumberOfDimensions(_matrix);
	if (dimensions > 2) {
	  LOG(ERROR) << "Only 2D matrices are supported";
	  return NULL;
	}
	if (mxIsDouble(_matrix)) {
	  LOG(ERROR) << "Cannot get non-mutated access to double content";
	  return NULL;
	} else if (mxIsSingle(_matrix)) {
	  return (float*) mxGetData(_matrix);
	} else {
	  LOG(ERROR) << "Only float and double matrices are supported";
	  return NULL;
	}
      } else {
	VLOG(2) << "Attempted to access non-matrix";
	return NULL;
      }

      return NULL;
    }

    string MatlabMatrix::GetStringContents() const {
      string contents;
      if (_matrix != NULL && _type == MATLAB_STRING) {
	const int rows = mxGetM(_matrix);
	const int cols = mxGetN(_matrix);
	const int length = rows > cols ? rows : cols;

	scoped_array<char> characters(new char[length+1]);
	if (mxGetString(_matrix, characters.get(), length+1) == 0) {
	  contents.assign(characters.get());
	} else {
	  // If the string is multibyte encoded, mxGetString will fail.
	  char* multibyte_string = mxArrayToString(_matrix);
	  if (multibyte_string) {
	    scoped_array<wchar_t> multibyte_characters(new wchar_t[length+1]);
	    mbstowcs(multibyte_characters.get(), multibyte_string, length+1);

	    for (int i = 0; i < wcslen(multibyte_characters.get()); ++i) {
	      const int byte = wctob(multibyte_characters[i]);
	      if (byte != EOF) {
		contents.append((const char*) &byte);
	      } else {
		contents.append("-");
	      }
	    }
	    mxFree(multibyte_string);
	  } else {
	    VLOG(1) << "Couldn't find string";
	  }
	}
      } else {
	VLOG(1) << "Attempted to access non-string matrix";
      }

      return contents;
    }

    MatlabMatrix MatlabMatrix::GetCopiedStructEntry(const int& row, const int& col) const {
      	const mwIndex subscripts[2] = {(mwIndex) row, (mwIndex) col};
	const int index = mxCalcSingleSubscript(_matrix, 2, subscripts);
	return GetCopiedStructEntry(index);
    }
    
    MatlabMatrix MatlabMatrix::GetCopiedStructEntry(const int& index) const {
      MatlabMatrix entry(MATLAB_STRUCT, Pair<int>(1,1));
      // Get a list of all of the fields in the contents.
      const vector<string> fields = GetStructFieldNames();
      for (int i = 0; i < (int) fields.size(); i++) {
	const string field = fields[i];
	entry.SetStructField(field, GetCopiedStructField(field, index));
      }

      return entry;
    }

    MatlabMatrix& MatlabMatrix::SetStructEntry(const int& row, const int& col, const MatlabMatrix& contents) {
      	const mwIndex subscripts[2] = {(mwIndex) row, (mwIndex) col};
	const int index = mxCalcSingleSubscript(_matrix, 2, subscripts);
	return SetStructEntry(index, contents);
    }

    MatlabMatrix& MatlabMatrix::SetStructEntry(const int& index, const MatlabMatrix& contents) {
      if (contents.GetMatrixType() != MATLAB_STRUCT) {
	LOG(ERROR) << "Attempted to access non-struct matrix";
	return (*this);
      }
      if (contents.GetNumberOfElements() != 1) {
	LOG(WARNING) << "There should only be one entry in the inserted struct";
      }
      // Get a list of all of the fields in the contents.
      const vector<string> fields = contents.GetStructFieldNames();
      for (int i = 0; i < (int) fields.size(); i++) {
	const string field = fields[i];
	SetStructField(field, index, contents.GetCopiedStructField(field));
      }

      return (*this);
    }

    MatlabMatrix& MatlabMatrix::SetStructField(const string& field, const int& row, const int& col, 
				      const MatlabMatrix& contents) {
      	const mwIndex subscripts[2] = {(mwIndex) row, (mwIndex) col};
	const int index = mxCalcSingleSubscript(_matrix, 2, subscripts);
	return SetStructField(field, index, contents);
    }

    MatlabMatrix& MatlabMatrix::SetStructField(const string& field, const MatlabMatrix& contents) {
      return SetStructField(field, 0, contents);
    }

    MatlabMatrix& MatlabMatrix::SetStructField(const string& field, const int& index, const MatlabMatrix& contents) {
      if (_matrix != NULL && _type == MATLAB_STRUCT) {
	// Check to see if the field already exists.
	if (mxGetFieldNumber(_matrix, field.c_str()) != -1) {
	  // If it exists, have to destroy it.
	  mxArray* data = mxGetField(_matrix, index, field.c_str());
	  mxDestroyArray(data);
	} else {
	  // Add the field to the struct (it may already be there).
	  mxAddField(_matrix, field.c_str());
	}

	if (contents._matrix == NULL) {
	  VLOG(3) << "Attempted to insert empty matrix into struct field: " << field 
		  << " (index: " << index << ")";
	  return (*this);
	}

	// Deep copy.
	mxArray* data = mxDuplicateArray(contents._matrix);
	mxSetField(_matrix, index, field.c_str(), data);
      } else {
	VLOG(2) << "Attempted to access non-struct (field: " << field << ")";
      }

      return (*this);
    }

    MatlabMatrix& MatlabMatrix::SetCell(const int& row, const int& col, const MatlabMatrix& contents) {
      	const mwIndex subscripts[2] = {(mwIndex) row, (mwIndex) col};
	const int index = mxCalcSingleSubscript(_matrix, 2, subscripts);
	return SetCell(index, contents);
    }

    MatlabMatrix& MatlabMatrix::SetCell(const int& index, const MatlabMatrix& contents) {
      if (_matrix != NULL && _type == MATLAB_CELL_ARRAY) {
	// Check to see if the cell has already been set.
	if (mxGetCell(_matrix, index) != NULL) {
	  // If it exists, we have to destroy it.
	  mxArray* data = mxGetCell(_matrix, index);
	  mxDestroyArray(data);
	}

	if (contents._matrix == NULL) {
	  VLOG(3) << "Attempted to insert empty matrix into cell: " << index;
	  return (*this);
	}

	// Deep copy.
	mxArray* data = mxDuplicateArray(contents._matrix);
	mxSetCell(_matrix, index, data);
      } else {
	VLOG(2) << "Attempted to access non-cell array (" << index << ")";
      }

      return *(this);
    }

    MatlabMatrix& MatlabMatrix::SetContents(const FloatMatrix& contents) {
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

      return (*this);
    }

    MatlabMatrix& MatlabMatrix::SetContents(const float* contents, const int& length, const bool& iscol) {
      const int rows = iscol ? length : 1;
      const int cols = iscol ? 1 : length;
      if (_type == MATLAB_MATRIX) {
	// Overwrite the already existing data if necessary.
	if (_matrix != NULL) {
	  mxDestroyArray(_matrix);
	}
	// Matlab matrices are in column-major order.
	_matrix = mxCreateNumericMatrix(rows, cols, mxSINGLE_CLASS, mxREAL);
	float* data = (float*) mxGetData(_matrix);
	memcpy(data, contents, sizeof(float) * length);
      } else {
	LOG(WARNING) << "Attempted to access non-matrix";
      }

      return (*this);
    }

    MatlabMatrix& MatlabMatrix::SetStringContents(const string& contents) {
      if (_type == MATLAB_STRING) {
	// Overwrite the already existing data if necessary.
	if (_matrix != NULL) {
	  mxDestroyArray(_matrix);
	}

	_matrix = mxCreateString(contents.c_str());
      } else {
	LOG(WARNING) << "Attempted to access non-string matrix";
      }

      return (*this);
    }
        
    void MatlabMatrix::LoadMatrixFromFile(const string& filename, const bool& multivariable) {
      MATFile* pmat = matOpen(filename.c_str(), "r");
      if (pmat == NULL) {
	LOG(ERROR) << "Error Opening MAT File: " << filename;
	return;
      }
      VLOG(1) << "Reading matrix from file: " << filename;

      if (multivariable) {
	const char* name = NULL;
	mxArray* data;

	_matrix = mxCreateStructMatrix(1, 1, 0, NULL);
	_type = MATLAB_STRUCT;

	while ((data = matGetNextVariable(pmat, &name)) != NULL) {
	  VLOG(1) << "Found variable " << name << " in file: " << filename;

	  SetStructField(name, 0, MatlabMatrix(data));
	  mxDestroyArray(data);
	}
      } else {
	// Should only have one entry.      
	const char* name = NULL;
	mxArray* data = matGetNextVariable(pmat, &name);
	VLOG(1) << "Found variable " << name << " in file: " << filename;
	if (matGetNextVariable(pmat, &name) != NULL) {
	  LOG(WARNING) << "Only one entry per MAT-file supported (name: " << name << ")";
	}

	_type = GetType(data);
#if 0
	_matrix = mxDuplicateArray(data);
	mxDestroyArray(data);
#else
	_matrix = data;
#endif
      }

      matClose(pmat);
    }

    bool MatlabMatrix::SaveToBinaryFile(const string& filename) const {
      FILE* fid = fopen(filename.c_str(), "wb");
      if (!fid) {
	LOG(ERROR) << "Could not open file for writing: " << filename;
	return false;
      }
      const string contents = Serialize();
      const int length = contents.length();
      fwrite(&length, sizeof(int), 1, fid);
      if (fwrite(contents.data(), sizeof(char), length, fid) != length) {
	LOG(ERROR) << "Error writing contents to file: " << filename;
	fclose(fid);
	return false;
      }
      fclose(fid);

      return true;
    }

    bool MatlabMatrix::SaveToFile(const string& filename, const bool& struct_format) const {
      MATFile* pmat = matOpen(filename.c_str(), "w");
      if (pmat == NULL) {
	LOG(ERROR) << "Error Opening MAT File: " << filename;
	return false;
      }
      VLOG(1) << "Writing matrix to file: " << filename;

      if (struct_format && GetMatrixType() == MATLAB_STRUCT && GetNumberOfElements() == 1) {
	vector<string> fields = GetStructFieldNames();
	for (int i = 0; i < (int) fields.size(); i++) {
	  VLOG(2) << "Writing output field to variable: " << fields[i];
	  const MatlabMatrix field = GetStructField(fields[i]);
	  if (field.GetNumberOfElements() == 0) {
	    continue;
	  }
	  if (_matrix == NULL || matPutVariable(pmat, fields[i].c_str(), field._matrix) != 0) {
	    LOG(ERROR) << "Error writing matrix data to file: " << filename << "(" << fields[i] << ")";
	    matClose(pmat);
	    return false;
	  }
	}
      } else {
	if (_matrix == NULL || matPutVariable(pmat, "data", _matrix) != 0) {
	  LOG(ERROR) << "Error writing matrix data to file: " << filename;
	  matClose(pmat);
	  return false;
	}
      }

      matClose(pmat);

      return true;
    }

    string MatlabMatrix::Serialize() const {
      stringstream ss(stringstream::out | stringstream::binary);

      switch(GetType(_matrix)) {
      case MATLAB_STRUCT: {
	// Indicate that we have a struct.
	ss.put('S');
	// Indicate the rows x columns. 3D NOT ALLOWED.
	const Pair<int> dimensions = GetDimensions();
	ss.write(reinterpret_cast<const char*>(&dimensions.x), sizeof(int));
	ss.write(reinterpret_cast<const char*>(&dimensions.y), sizeof(int));
	// Get the list of fields and write to stream.
	vector<string> field_names = GetStructFieldNames();
	const int field_names_length = field_names.size();
	ss.write(reinterpret_cast<const char*>(&field_names_length), sizeof(int));
	for (uint32 i = 0; i < field_names.size(); i++) {
	  const string field = field_names[i];
	  const int field_length = field.length();
	  ss.write(reinterpret_cast<const char*>(&field_length), sizeof(int));
	  ss.write(field.data(), field.length());
	}

	// Now write out the actual data.
	const int length = dimensions.x * dimensions.y;
	for (uint32 i = 0; i < field_names.size(); i++) {
	  const string field = field_names[i];
	  for (int j = 0; j < length; j++) {
	    const int index = j;
	    // Serialize each field to the stream.
	    string field_serialized = GetCopiedStructField(field, index).Serialize();
	    ss.write(field_serialized.data(), field_serialized.length());
	  }
	}
	break;
      }
      case MATLAB_CELL_ARRAY: {
	// Indicate that we have a cell array.
	ss.put('C');
	// Indicate the rows x columns. 3D NOT ALLOWED.
	const Pair<int> dimensions = GetDimensions();
	ss.write(reinterpret_cast<const char*>(&dimensions.x), sizeof(int));
	ss.write(reinterpret_cast<const char*>(&dimensions.y), sizeof(int));
	// Write out the actual data.
	const int length = dimensions.x * dimensions.y;
	for (int j = 0; j < length; j++) {
	  const int index = j;
	  // Serialize each field to the stream.
	  string cell_serialized = GetCopiedCell(index).Serialize();
	  ss.write(cell_serialized.data(), cell_serialized.length());
	}
	break;
      }
      case MATLAB_MATRIX: {
	// Indicate that we have a matrix.
	ss.put('M');
	// Indicate the rows x columns. 3D NOT ALLOWED.
	const Pair<int> dimensions = GetDimensions();
	ss.write(reinterpret_cast<const char*>(&dimensions.x), sizeof(int));
	ss.write(reinterpret_cast<const char*>(&dimensions.y), sizeof(int));
	// Write out the actual data.
	const FloatMatrix contents = GetCopiedContents();
	const int length = contents.rows() * contents.cols();
	ss.write(reinterpret_cast<const char*>(contents.data()), sizeof(float) * length);
	break;
      }
      case MATLAB_STRING: {
	// Indicate we have a string.
	ss.put('Z');
	const Pair<int> dimensions = GetDimensions();
	ss.write(reinterpret_cast<const char*>(&dimensions.x), sizeof(int));
	ss.write(reinterpret_cast<const char*>(&dimensions.y), sizeof(int));
	// Write out the actual data.
	const string contents = GetStringContents();
	VLOG(2) << "Writing string: " << contents;
	ss.write(contents.c_str(), sizeof(char) * (contents.length() + 1));
	break;
      }
      default:
	// In this case, we assume an empty matrix, and indicate that.
	ss.put('E');
	break;
      }
      ss.flush();
      return ss.str();
    }

    long long int MatlabMatrix::Deserialize(const string& str, const long long int& position) {
      // These methods mirror the above methods.
      const char* ss = str.c_str() + position;

      // Read the first element, it determines the root matrix type.
      char type;
      memcpy(&type, ss, sizeof(char)); ss += sizeof(char);

      long long int offset = position + sizeof(char);
      switch (type) {
      case 'S': {  // Struct
	VLOG(2) << "Found struct at offset: " << offset;
	_type = MATLAB_STRUCT;

	// Size of the struct.
	Pair<int> dimensions;
	memcpy(&dimensions.x, ss, sizeof(int)); ss += sizeof(int);
	memcpy(&dimensions.y, ss, sizeof(int)); ss += sizeof(int);
	offset += sizeof(int) * 2;
	// List of fields.
	int num_fields;
	memcpy(&num_fields, ss, sizeof(int)); ss += sizeof(int);
	offset += sizeof(int) * 1;

	vector<string> fields;
	for (int i = 0; i < num_fields; i++) {
	  int field_length;
	  memcpy(&field_length, ss, sizeof(int)); ss += sizeof(int);
	  offset += sizeof(int) * 1;

	  scoped_array<char> field_cstr(new char[field_length]);
	  memcpy(field_cstr.get(), ss, sizeof(char) * field_length); ss += sizeof(char) * field_length;
	  offset += sizeof(char) * field_length;

	  fields.push_back(string(field_cstr.get(), field_length));
	}
	// We have to create this array for when we create the struct.
	scoped_array<const char*> fields_cstr(new const char*[num_fields]);
	for (uint i = 0; i < fields.size(); i++) {
	  fields_cstr[i] = fields[i].c_str();
	}
	// Initialize the _matrix to be a struct of the correct layout.
	_matrix = mxCreateStructMatrix(dimensions.x, dimensions.y, num_fields, fields_cstr.get());
	// And now the actual data.
	const int length = dimensions.x * dimensions.y;
	for (uint32 i = 0; i < fields.size(); i++) {
	  const string field = fields[i];
	  VLOG(2) << "Reading field: " << field;
	  for (int j = 0; j < length; j++) {
	    const int index = j;
	    // Deserialize the field and then save it.
	    MatlabMatrix field_matrix;
	    const long long int bytes_read = field_matrix.Deserialize(str, offset);
	    if (bytes_read == 0L) {
	      LOG(ERROR) << "Malformed field: " << field << " (index: " << index << ")";
#if 1
#else
	      return 0;
#endif
	    }
	    offset += bytes_read;
	    SetStructField(field, index, field_matrix);
	  }
	}
	break;
      }
      case 'C': {  // Cell Array
	VLOG(2) << "Found cell array at offset: " << offset;
	_type = MATLAB_CELL_ARRAY;

	// Size of the cell array.
	Pair<int> dimensions;
	memcpy(&dimensions.x, ss, sizeof(int)); ss += sizeof(int);
	memcpy(&dimensions.y, ss, sizeof(int)); ss += sizeof(int);
	offset += sizeof(int) * 2;
	// Initialize the _matrix to be a cell array.
	_matrix = mxCreateCellMatrix(dimensions.x, dimensions.y);
	// And now the actual data.
	const int length = dimensions.x * dimensions.y;
	for (int j = 0; j < length; j++) {
	  const int index = j;
	  // Deserialize the field and then save it.
	  MatlabMatrix cell_matrix;
	  const long long int bytes_read = cell_matrix.Deserialize(str, offset);
	  if (bytes_read == 0L) {
	    LOG(ERROR) << "Malformed cell at index: " << index;
	    return 0;
	  }
	  offset += bytes_read;
	  SetCell(index, cell_matrix);
	}
	break;
      }
      case 'M': {  // Matrix
	VLOG(2) << "Found matrix at offset: " << offset;
	_type = MATLAB_MATRIX;

	// Size of the matrix.
	Pair<int> dimensions;
	memcpy(&dimensions.x, ss, sizeof(int)); ss += sizeof(int);
	memcpy(&dimensions.y, ss, sizeof(int)); ss += sizeof(int);
	offset += sizeof(int) * 2;
	// No initialization necessary as the SetContents method takes care of it.
	// And now the actual data.
	const int rows = dimensions.x;
	const int cols = dimensions.y;
	VLOG(2) << "Matrix size: " << rows << " x " << cols;
	FloatMatrix contents(rows, cols);
	contents.fill(0.0f);

	memcpy(contents.data(), ss, sizeof(float) * rows * cols); ss += sizeof(float) * rows * cols;
	offset += sizeof(float) * rows * cols;
	SetContents(contents);
	break;
      }
      case 'Z': {  // String
	VLOG(2) << "Found string at offset: " << offset;
	_type = MATLAB_STRING;

	// Size of the matrix.
	Pair<int> dimensions;
	memcpy(&dimensions.x, ss, sizeof(int)); ss += sizeof(int);
	memcpy(&dimensions.y, ss, sizeof(int)); ss += sizeof(int);
	offset += sizeof(int) * 2;
	// No initialization necessary as the SetContents method takes care of it.
	// And now the actual data.
	const int rows = dimensions.x;
	const int cols = dimensions.y;
	const int length = rows > cols ? rows : cols;
	VLOG(2) << "String length: " << length;

	scoped_array<char> characters(new char[length+1]);
	memcpy(characters.get(), ss, sizeof(char) * (length + 1)); ss += sizeof(char) * (length + 1);
	offset += sizeof(char) * (length + 1);
	SetStringContents(string(characters.get()));
	break;
      }
      case 'E': {  // No type
	VLOG(2) << "Found empty matrix at offset: " << offset;
	_type = MATLAB_NO_TYPE;
	break;
      }
      default:
	LOG(ERROR) << "Unknown matrix type: " << type << " (Offset: " << offset << ")";
	//return 0;
      }

      return (offset - position);
    }

    /**
       MatlabConverter 
     **/

    int MatlabConverter::_matlab_offset = 1;

    void MatlabConverter::EnableMatlabOffset() {
      _matlab_offset = 1;
    }

    void MatlabConverter::DisableMatlabOffset() {
      _matlab_offset = 0;
    }

    MatlabMatrix MatlabConverter::ConvertModelToMatrix(const Model& model) {
      MatlabMatrix matrix(MATLAB_STRUCT, Pair<int>(1, 1));

      matrix.SetStructField("rho", MatlabMatrix(model.rho));
      matrix.SetStructField("w", MatlabMatrix(model.weights.get(), 1, model.num_weights));
      matrix.SetStructField("firstLabel", MatlabMatrix(model.first_label));
      
      MatlabMatrix info(MATLAB_STRUCT, Pair<int>(1, 1));
      info.SetStructField("numPositives", MatlabMatrix((float) model.num_positives));
      info.SetStructField("numNegatives", MatlabMatrix((float) model.num_negatives));
      matrix.SetStructField("info", info);

      matrix.SetStructField("threshold", MatlabMatrix(model.threshold));

      return matrix;
    }
    
    MatlabMatrix MatlabConverter::ConvertMetadataToMatrix(const vector<DetectionMetadata>& metadata,
							  const bool& minimal) {
      MatlabMatrix matrix(MATLAB_STRUCT, Pair<int>(1, metadata.size()));

      for (int i = 0; i < (int) metadata.size(); i++) {
	const DetectionMetadata entry = metadata[i];
	matrix.SetStructField("im", i, MatlabMatrix(entry.image_path));
	matrix.SetStructField("x1", i, MatlabMatrix((float) entry.x1 + _matlab_offset));
	matrix.SetStructField("x2", i, MatlabMatrix((float) entry.x2 + _matlab_offset));
	matrix.SetStructField("y1", i, MatlabMatrix((float) entry.y1 + _matlab_offset));
	matrix.SetStructField("y2", i, MatlabMatrix((float) entry.y2 + _matlab_offset));

	if (minimal) {
	  continue;
	}
	matrix.SetStructField("flip", i, MatlabMatrix(0.0f));
	matrix.SetStructField("trunc", i, MatlabMatrix(0.0f));

	MatlabMatrix size(MATLAB_STRUCT, Pair<int>(1, 1));
	size.SetStructField("ncols", MatlabMatrix(entry.image_size.x));
	size.SetStructField("nrows", MatlabMatrix(entry.image_size.y));
	matrix.SetStructField("size", i, size);

	matrix.SetStructField("imidx", i, MatlabMatrix((float) entry.image_index + _matlab_offset));
	matrix.SetStructField("setidx", i, MatlabMatrix((float) entry.image_set_index + _matlab_offset));

	// The original order is <level, x, y>, but MATLAB expects <level, y, x>. Also have to add one.
	FloatMatrix pyramid_offset(1, 3);
	pyramid_offset << 
	  (float) entry.pyramid_offset.x + _matlab_offset
	  , (float) entry.pyramid_offset.z + _matlab_offset
	  , (float) entry.pyramid_offset.y + _matlab_offset;
	matrix.SetStructField("pyramid", i, MatlabMatrix(pyramid_offset));
      }

      return matrix;
    }

    MatlabMatrix MatlabConverter::ConvertDetectionsToMatrixSimplified(const DetectionResultSet& detections,
								      const vector<int>& image_indices,
								      const vector<int>& assigned_clusters) {
      int total_entries = 0;
      for (int i = 0; i < (int) detections.model_detections.size(); i++) {
	for (int j = 0; j < (int) detections.model_detections[i].detections.size(); j++) {
	  total_entries++;
	}
      }
      MatlabMatrix matrix(MATLAB_STRUCT, Pair<int>(total_entries, 1));

      int current_index = 0;
      for (int i = 0; i < (int) detections.model_detections.size(); i++) {
	for (int j = 0; j < (int) detections.model_detections[i].detections.size(); j++) {
	  const DetectionMetadata metadata = detections.model_detections[i].detections[j].metadata;
	  MatlabMatrix position(MATLAB_STRUCT, Pair<int>(1,1));
	  position.SetStructField("x1", MatlabMatrix((float) metadata.x1 + _matlab_offset));
	  position.SetStructField("x2", MatlabMatrix((float) metadata.x2 + _matlab_offset));
	  position.SetStructField("y1", MatlabMatrix((float) metadata.y1 + _matlab_offset));
	  position.SetStructField("y2", MatlabMatrix((float) metadata.y2 + _matlab_offset));

	  int current_image_index = 0;
	  if (image_indices.size() == 0) {
	    current_image_index = metadata.image_index;
	  } else {
	    if (image_indices.size() == 1) {
	      current_image_index = image_indices[0];
	    } else {
	      current_image_index = image_indices[current_index];
	    }
	  }

	  int detector = i;
	  if (assigned_clusters.size() != 0) {
	    detector = assigned_clusters[current_image_index];
	  }

	  matrix.SetStructField("decision", current_index, 
				MatlabMatrix(detections.model_detections[i].detections[j].score));
	  matrix.SetStructField("pos", current_index, position);
	  matrix.SetStructField("imidx", current_index, 
				MatlabMatrix((float) (current_image_index + _matlab_offset)));
	  matrix.SetStructField("detector", current_index, MatlabMatrix((float) (detector + _matlab_offset)));

	  if (detections.model_detections[i].features.rows() > 0 
	      && detections.model_detections[i].features.cols()) {
	    matrix.SetStructField("features", current_index, 
				  MatlabMatrix(detections.model_detections[i].features.row(j)));
	  }
	  current_index++;
	}
      }
      return matrix;
    }

    Detector MatlabConverter::ConvertMatrixToDetector(const MatlabMatrix& matrix) {
      return DetectorFactory::InitializeFromMatlabArray(matrix.GetMatlabArray());
    }

    MatlabMatrix MatlabConverter::ConvertDetectorToMatrix(const Detector& detector) {
      MatlabMatrix matrix(MATLAB_STRUCT, Pair<int>(1,1));

      MatlabMatrix firstLevModels(MATLAB_STRUCT, Pair<int>(1,1));
      firstLevModels.SetStructField("w", MatlabMatrix(detector.ComputeWeightMatrix().transpose()));
      firstLevModels.SetStructField("rho", MatlabMatrix(detector.ComputeOffsetsMatrix().transpose()));
      firstLevModels.SetStructField("firstLabel", MatlabMatrix(detector.ComputeLabelsMatrix().transpose()));

      vector<float> thresholds(detector.GetNumModels());
      for (int i = 0; i < (int) thresholds.size(); i++) {
	thresholds[i] = detector.GetModel(i).threshold;
      }
      firstLevModels.SetStructField("threshold", MatlabMatrix(thresholds));
      firstLevModels.SetStructField("type", MatlabMatrix("composite"));

      matrix.SetStructField("firstLevModels", firstLevModels);

      mxArray* params;
      detector.SaveParametersToMatlabMatrix(&params);
      matrix.SetStructField("params", MatlabMatrix(params));
      mxDestroyArray(params);

      return matrix;
    }

  }  // namespace util
}  // namespace slib
  
