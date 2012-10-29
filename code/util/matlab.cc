#include "matlab.h"

#include <common/types.h>
#include <glog/logging.h>
#include <iostream>
#include <mat.h>
#include <string>
#include <sstream>
#include <vector>

using slib::svm::Model;
using std::string;
using std::stringstream;
using std::vector;

namespace slib {
  namespace util {

    MatlabMatrix::MatlabMatrix() 
      : _matrix(NULL)
      , _type(MATLAB_NO_TYPE) {}
    
    MatlabMatrix::MatlabMatrix(const MatlabMatrixType& type) 
      : _matrix(NULL)
      , _type(type) {}

    MatlabMatrix::MatlabMatrix(const MatlabMatrixType& type, const Pair<int>& dimensions) 
      : _matrix(NULL)
      , _type(type) {
      if (_type == MATLAB_STRUCT) {
	_matrix = mxCreateStructMatrix(dimensions.x, dimensions.y, 0, NULL);
      } else if (_type == MATLAB_CELL_ARRAY) {
	_matrix = mxCreateCellMatrix(dimensions.x, dimensions.y);
      } else if (_type == MATLAB_MATRIX) {
	_matrix = mxCreateNumericMatrix(dimensions.x, dimensions.y, mxSINGLE_CLASS, mxREAL);
      }
    }

    MatlabMatrix::~MatlabMatrix() {
      if (_matrix != NULL) {
	mxDestroyArray(_matrix);
      }
      _matrix = NULL;
    }

    MatlabMatrix::MatlabMatrix(const mxArray* data)
      : _matrix(NULL)
      , _type(MATLAB_NO_TYPE) {
      if (data != NULL) {
	_type = GetType(data);
	_matrix = mxDuplicateArray(data);
      }
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
    
    MatlabMatrix::MatlabMatrix(const float& value)
      : _matrix(NULL)
      , _type(MATLAB_MATRIX) {
      FloatMatrix contents(1,1);
      contents(0,0) = value;
      SetContents(contents);
    }
    
    MatlabMatrix::MatlabMatrix(const float* contents, const int& rows, const int& cols) 
      : _matrix(NULL)
      , _type(MATLAB_MATRIX) {
      FloatMatrix matrix(rows, cols);
      memcpy(matrix.data(), contents, sizeof(float) * rows * cols);
      SetContents(matrix);
    }

    MatlabMatrix::MatlabMatrix(const FloatMatrix& contents) 
      : _matrix(NULL)
      , _type(MATLAB_MATRIX) {
      SetContents(contents);
    }

    const MatlabMatrix& MatlabMatrix::operator=(const MatlabMatrix& right) {
      if (_matrix != NULL) {
	mxDestroyArray(_matrix);
      }
      _matrix = mxDuplicateArray(right._matrix);
      _type = right._type;

      return (*this);
    }

    MatlabMatrix& MatlabMatrix::Merge(const MatlabMatrix& other) {
      if (_type != other._type) {
	return *this;
      }
      if (GetDimensions() != other.GetDimensions) {
	return *this;
      }
    }

    MatlabMatrixType MatlabMatrix::GetType(const mxArray* data) const {
      if (data == NULL) {
	return MATLAB_NO_TYPE;
      }
      if (mxIsNumeric(data)) {
	return MATLAB_MATRIX;
      } else if (mxIsStruct(data)) {
	return MATLAB_STRUCT;
      } else if (mxIsCell(data)) {
	return MATLAB_CELL_ARRAY;
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
    
    MatlabMatrix MatlabMatrix::LoadFromFile(const string& filename) {
      MatlabMatrix matrix(filename);
      return matrix;
    }
    
    MatlabMatrix MatlabMatrix::GetStructField(const string& field, const int& index) const {
      if (_matrix != NULL && _type == MATLAB_STRUCT) {
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
      	const mwIndex subscripts[2] = {row, col};
	const int index = mxCalcSingleSubscript(_matrix, 2, subscripts);
	return GetCell(index);
    }

    MatlabMatrix MatlabMatrix::GetCell(const int& index) const {
      if (_matrix != NULL && _type == MATLAB_CELL_ARRAY) {
	const mxArray* data = mxGetCell(_matrix, index);
	return MatlabMatrix(data);
      } else {
	LOG(WARNING) << "Attempted to access non-cell array (" << index << ")";
	return MatlabMatrix(MATLAB_NO_TYPE);
      }
    }

    FloatMatrix MatlabMatrix::GetContents() const {
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
	  LOG(WARNING) << "Attempted to insert empty metrix into struct field: " << field 
		       << " (index: " << index << ")";
	  return;
	}

	// Deep copy.
	mxArray* data = mxDuplicateArray(contents._matrix);
	mxSetField(_matrix, index, field.c_str(), data);
      } else {
	LOG(WARNING) << "Attempted to access non-struct (field: " << field << ")";
      }
    }

    void MatlabMatrix::SetCell(const int& row, const int& col, const MatlabMatrix& contents) {
      	const mwIndex subscripts[2] = {row, col};
	const int index = mxCalcSingleSubscript(_matrix, 2, subscripts);
	SetCell(index, contents);
    }

    void MatlabMatrix::SetCell(const int& index, const MatlabMatrix& contents) {
      if (_matrix != NULL && _type == MATLAB_CELL_ARRAY) {
	// Check to see if the cell has already been set.
	if (mxGetCell(_matrix, index) != NULL) {
	  // If it exists, we have to destroy it.
	  mxArray* data = mxGetCell(_matrix, index);
	  mxDestroyArray(data);
	}

	if (contents._matrix == NULL) {
	  LOG(WARNING) << "Attempted to insert empty metrix into cell: " << index;
	  return;
	}

	// Deep copy.
	mxArray* data = mxDuplicateArray(contents._matrix);
	mxSetCell(_matrix, index, data);
      } else {
	LOG(WARNING) << "Attempted to access non-cell array (" << index << ")";
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
	    string field_serialized = GetStructField(field, index).Serialize();
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
	  string cell_serialized = GetCell(index).Serialize();
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
	const FloatMatrix contents = GetContents();
	const int length = contents.rows() * contents.cols();
	ss.write(reinterpret_cast<const char*>(contents.data()), sizeof(float) * length);
	break;
      }
      default:
	break;
      }
      ss.flush();
      return ss.str();
    }

    int MatlabMatrix::Deserialize(const string& str, const int& position) {
      // These methods mirror the above methods.
      stringstream ss(str, stringstream::in | stringstream::binary);
      ss.seekg(position);

      // Read the first element, it determines the root matrix type.
      char type;
      ss.get(type);

      int offset = position + sizeof(char);
      switch (type) {
      case 'S': {  // Struct
	VLOG(1) << "Found struct at offset: " << offset;
	_type = MATLAB_STRUCT;

	// Size of the struct.
	Pair<int> dimensions;
	ss.read(reinterpret_cast<char*>(&dimensions.x), sizeof(int));
	ss.read(reinterpret_cast<char*>(&dimensions.y), sizeof(int));
	offset += sizeof(int) * 2;
	// List of fields.
	int num_fields;
	ss.read(reinterpret_cast<char*>(&num_fields), sizeof(int));
	offset += sizeof(int) * 1;
	vector<string> fields;
	for (int i = 0; i < num_fields; i++) {
	  int field_length;
	  ss.read(reinterpret_cast<char*>(&field_length), sizeof(int));
	  offset += sizeof(int) * 1;
	  scoped_ptr<char> field_cstr(new char[field_length]);
	  ss.read(field_cstr.get(), field_length);
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
	  VLOG(1) << "Reading field: " << field;
	  for (int j = 0; j < length; j++) {
	    const int index = j;
	    // Deserialize the field and then save it.
	    MatlabMatrix field_matrix;
	    const int bytes_read = field_matrix.Deserialize(str, offset);
	    if (bytes_read == 0) {
	      LOG(ERROR) << "Malformed field: " << field << " (index: " << index << ")";
	      return 0;
	    }
	    offset += bytes_read;
	    SetStructField(field, index, field_matrix);
	  }
	}
	break;
      }
      case 'C': {  // Cell Array
	VLOG(1) << "Found cell array at offset: " << offset;
	_type = MATLAB_CELL_ARRAY;

	// Size of the cell array.
	Pair<int> dimensions;
	ss.read(reinterpret_cast<char*>(&dimensions.x), sizeof(int));
	ss.read(reinterpret_cast<char*>(&dimensions.y), sizeof(int));
	offset += sizeof(int) * 2;
	// Initialize the _matrix to be a cell array.
	_matrix = mxCreateCellMatrix(dimensions.x, dimensions.y);
	// And now the actual data.
	const int length = dimensions.x * dimensions.y;
	for (int j = 0; j < length; j++) {
	  const int index = j;
	  // Deserialize the field and then save it.
	  MatlabMatrix cell_matrix;
	  const int bytes_read = cell_matrix.Deserialize(str, offset);
	  if (bytes_read == 0) {
	    LOG(ERROR) << "Malformed cell at index: " << index;
	    return 0;
	  }
	  offset += bytes_read;
	  SetCell(index, cell_matrix);
	}
	break;
      }
      case 'M': {  // Matrix
	VLOG(1) << "Found matrix at offset: " << offset;
	_type = MATLAB_MATRIX;

	// Size of the matrix.
	Pair<int> dimensions;
	ss.read(reinterpret_cast<char*>(&dimensions.x), sizeof(int));
	ss.read(reinterpret_cast<char*>(&dimensions.y), sizeof(int));
	offset += sizeof(int) * 2;
	// No initialization necessary as the SetContents method takes care of it.
	// And now the actual data.
	const int rows = dimensions.x;
	const int cols = dimensions.y;
	VLOG(2) << "Matrix size: " << rows << " x " << cols;
	FloatMatrix contents(rows, cols);
	contents.fill(0.0f);

	ss.read(reinterpret_cast<char*>(contents.data()), sizeof(float) * rows * cols);
	offset += sizeof(float) * rows * cols;
	SetContents(contents);
	break;
      }
      default:
	LOG(ERROR) << "Unknown matrix type: " << type;
	return 0;
      }

      return offset - position;
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
  }  // namespace util
}  // namespace slib
  
