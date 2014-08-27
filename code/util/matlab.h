#ifndef __SLIB_UTIL_MATLAB_H__
#define __SLIB_UTIL_MATLAB_H__

#include <CImg.h>
#include <common/scoped_ptr.h>
#include <common/types.h>
#undef Success
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <glog/logging.h>
#include <map>
#include <mat.h>
#include <iostream>
#include <string>
#include <sstream>

namespace slib {
  namespace svm {
    class Detector;
    struct DetectionMetadata;
    struct DetectionResultSet;
    struct Model;
  }
}

namespace slib {
  namespace util {

    // TODO(sean): Create types MATLAB_FLOAT_MATRIX, etc. so that we
    // can avoid the calls to mxIsDouble, etc.
    enum MatlabMatrixType {
      MATLAB_STRUCT, MATLAB_CELL_ARRAY, MATLAB_MATRIX, 
      MATLAB_STRING, MATLAB_NO_TYPE, MATLAB_MATRIX_SPARSE
    };

    /**
       This class is an abstraction of the MATLAB matrix type. It is
       quite simplified since I never need the more advanced
       functionality. 

       This class can really be thought of as a tree of "matrices",
       each of which has the type MatlabMatrix. All leaves are actual
       matrices and should be retrieved via GetContents. Any branch
       node is either a struct or a cell and can be indexed
       accordingly.

       This class works best when you know the format of your data a
       prior (which is almost always the case).

       This class is STL container compatible.

       When you retrieve one of the nodes in the "tree", its subtree
       is COPIED to the output. This can be expensive, but allows me
       to care very little about trying to manage shared access to
       different parts of the tree simultaneously.

       Lastly, you can serialize this class to and from a byte stream,
       which means it can be transmitted across any arbitrary channel
       (MPI in particular).
     */
    class MatlabMatrix {
    public:
      MatlabMatrix();
      // Cannot be explicit for a number of reasons.
      MatlabMatrix(const MatlabMatrix& matrix);

      explicit MatlabMatrix(const MatlabMatrixType& type);
      MatlabMatrix(const MatlabMatrixType& type, const Pair<int>& dimensions);
      MatlabMatrix(const MatlabMatrixType& type, const int& rows, const int& cols);      
      virtual ~MatlabMatrix();
      // Creates a character array with the contents.
      explicit MatlabMatrix(const std::string& contents);

      // STL vector compatible contructor. Usually the compiler can infer the type automatically.
      template <typename T>
      explicit MatlabMatrix(const std::vector<T>& values, const bool& col = true) 
	: _matrix(NULL), _type(MATLAB_MATRIX) {
	if (col) {
	  FloatMatrix matrix(values.size(), 1);
	  for (int i = 0; i < (int) values.size(); i++) {
	    matrix(i, 0) = static_cast<float>(values[i]);
	  }
	  SetContents(matrix);
	} else {
	  FloatMatrix matrix(1, values.size());
	  for (int i = 0; i < (int) values.size(); i++) {
	    matrix(0, i) = static_cast<float>(values[i]);
	  }
	  SetContents(matrix);
	}
      }

      // This is a pseudo-specialization of the above constructor
      // vectors of strings. Note that we aren't actually specializing
      // the template.
      MatlabMatrix(const std::vector<std::string>& values, const bool& col = true);

      // For scalar values.
      explicit MatlabMatrix(const float& data);
      // Pointers to data.
      MatlabMatrix(const float* contents, const int& rows, const int& cols);
      // Compatibility with Eigen.
      explicit MatlabMatrix(const FloatMatrix& contents);

      // Very important for STL compatibility.
      const MatlabMatrix& operator=(const MatlabMatrix& right);

      // This is a merge, but it is rather strict. It requires both
      // matrices to be the same type and the same size. In the event
      // that both matrices define the same index / field, the "this"
      // matrix will be given priority.
      MatlabMatrix& Merge(const MatlabMatrix& other);

      // This is a useful function for assignment. You should really
      // avoid using the operator= method as it tends to produce
      // inconsistent results across different platforms. This method
      // is much safer.
      void Assign(const MatlabMatrix& other);

      static MatlabMatrix LoadFromFile(const std::string& filename, const bool& multivariable = false);
      static MatlabMatrix LoadFromBinaryFile(const std::string& filename);
      bool SaveToFile(const std::string& filename, const bool& struct_format = false) const;
      bool SaveToBinaryFile(const std::string& filename) const;

      bool HasStructField(const std::string& file, const int& index = 0) const;
      bool HasStructField(const std::string& file, const int& row, const int& col) const;

      // TODO(sarietta): Slowly transition this to be GetMutable* and Get*.

      MatlabMatrix GetCopiedStructField(const std::string& field, const int& index = 0) const;
      MatlabMatrix GetCopiedStructField(const std::string& field, const int& row, const int& col) const;
      // Gets the entire struct at the index. In MATLAB for a struct A, A(index).
      MatlabMatrix GetCopiedStructEntry(const int& index = 0) const;
      MatlabMatrix GetCopiedStructEntry(const int& row, const int& col) const;

      MatlabMatrix GetCopiedCell(const int& row, const int& col) const;
      MatlabMatrix GetCopiedCell(const int& index) const;
      FloatMatrix GetCopiedContents() const;
      SparseFloatMatrix GetCopiedSparseContents() const;

      // Non-mutator access. Should be faster and more memory efficient.
      const MatlabMatrix GetStructField(const std::string& field, const int& index = 0) const;
      const MatlabMatrix GetStructField(const std::string& field, const int& row, const int& col) const;
      const MatlabMatrix GetCell(const int& row, const int& col) const;
      const MatlabMatrix GetCell(const int& index) const;
      // No bounds checking happens on the next two methods.
      float GetMatrixEntry(const int& row, const int& col) const;
      float GetMatrixEntry(const int& index) const;
      const float* GetContents() const;

      // Mutable access. Use these at your own risk. You can seriously
      // corrupt the hierarchy of the matrices if you mess around.
      void GetMutableCell(const int& index, MatlabMatrix* cell) const;
      void GetMutableCell(const int& row, const int& col, MatlabMatrix* cell) const;
      //void GetMutableStructField(const std::string& field, const int& index, MatlabMatrix* struct_field) const;

      float GetScalar() const;
      std::string GetStringContents() const;

      MatlabMatrix& SetStructField(const std::string& field, const MatlabMatrix& contents);
      MatlabMatrix& SetStructField(const std::string& field, const int& index, const MatlabMatrix& contents);
      MatlabMatrix& SetStructField(const std::string& field, const int& row, const int& col, 
				   const MatlabMatrix& contents);
      // Sets the entire struct at the specified index. 
      MatlabMatrix& SetStructEntry(const int& index, const MatlabMatrix& contents);
      MatlabMatrix& SetStructEntry(const int& row, const int& col, const MatlabMatrix& contents);

      MatlabMatrix& SetCell(const int& row, const int& col, const MatlabMatrix& contents);
      MatlabMatrix& SetCell(const int& index, const MatlabMatrix& contents);

      // This method will convert a MATLAB_CELL_ARRAY to a matrix. All
      // non-MATLAB_NO_TYPE cells must be the same type. Additionally,
      // if the cells have type MATLAB_MATRIX, all of the cells must
      // contain the same number of columns (although 0 columns is
      // fine).
      //
      // The resulting matrix will either be a MATLAB_MATRIX or a
      // MATLAB_STRUCT depending on what the cells contain. <em>In
      // both cases, the cell array is flattened in column-major
      // order.</em> The total size of the resulting matrix will be
      // (total_rows, cols), where total_rows = \sum_i^N cell_i.rows
      // and cols = cell_i.cols \forall i.
      MatlabMatrix CellToMatrix() const;

      // This is what you would expect to call for a 'normal' matrix,
      // and it's relatively efficient all things considered, but it's
      // probably best if you can initialize larger chunks in
      // FloatMatrix and then set the contents.
      //
      // Note this only works for regular matrices, not cell nor
      // struct.
      template <typename T>
      inline MatlabMatrix& Set(const int& row, const int& col, const T& value) {
	return SetMatrixEntry(row, col, value);
      }

      template <typename T>
      inline MatlabMatrix& SetMatrixEntry(const int& row, const int& col, const T& value) {
	const mwIndex subscripts[2] = {(mwIndex) row, (mwIndex) col};
	const int index = mxCalcSingleSubscript(_matrix, 2, subscripts);
	return SetMatrixEntry(index, value);
      }

      template <typename T>
      inline MatlabMatrix& SetMatrixEntry(const int& index, const T& value) {
	if (_matrix != NULL && _type == MATLAB_MATRIX) {
	  if (mxIsDouble(_matrix)) {
	    ((double*) mxGetData(_matrix))[index] = static_cast<double>(value);
	  } else if (mxIsSingle(_matrix)) {
	    ((float*) mxGetData(_matrix))[index] = static_cast<float>(value);
	  } else {
	    LOG(ERROR) << "Only float and double matrices are supported";
	  }
	} else {
	  VLOG(2) << "Attempted to access non-matrix";
	}
	
	return (*this);
      }

      MatlabMatrix& SetContents(const FloatMatrix& contents);
      // iscol = is this a column-vector, i.e. rows = length
      MatlabMatrix& SetContents(const float* contents, const int& length, const bool& iscol = false);
      MatlabMatrix& SetStringContents(const std::string& contents);

      MatlabMatrix& SetScalar(const float& scalar);

      // Although the return type is a "string", the contents of that
      // string will be fwrite-style bytes.
      std::string Serialize() const;
      // The stream knows its own length, however this function is
      // recursive and needs to be able to start reading from the
      // correct position in the stream. A calling method does not
      // need to worry with these details... just use the default.
      long long int Deserialize(const std::string& str, const long long int& position = 0L);

      Pair<int> GetDimensions() const;
      std::vector<std::string> GetStructFieldNames() const;
      
      inline int GetNumberOfElements() const {
	const Pair<int> dimensions = GetDimensions();
	return (dimensions.x * dimensions.y);
      }

      inline int size() const {
	return GetNumberOfElements();
      }

      // Try to avoid this. It's more efficient to get them both via
      // GetDimensions().
      inline int cols() const {
	const Pair<int> dimensions = GetDimensions();
	return dimensions.y;
      }

      // Try to avoid this. It's more efficient to get them both via
      // GetDimensions().
      inline int rows() const {
	const Pair<int> dimensions = GetDimensions();
	return dimensions.x;
      }

      inline MatlabMatrixType GetMatrixType() const {
	return _type;
      }

      // Only use this if you know what you're doing. Seriously.
      inline const mxArray& GetMatlabArray() const {
	return (*_matrix);
      }

      // These are nice methods that call the correct get/set
      // functions for you. Try to avoid them when you can. Also note
      // that they only work for cells and structs.
      inline MatlabMatrix Get(const int& row, const int& col) const {
	if (_type == MATLAB_CELL_ARRAY) {
	  return GetCopiedCell(row, col);
	} else if (_type == MATLAB_STRUCT) {
	  return GetCopiedStructEntry(row, col);
	} else if (_type == MATLAB_MATRIX) {
	  return MatlabMatrix(GetCopiedContents()(row, col));
	} else {
	  return MatlabMatrix();
	}
      }

      inline MatlabMatrix& Set(const int& row, const int& col, const MatlabMatrix& contents) {
	if (_type == MATLAB_CELL_ARRAY) {
	  SetCell(row, col, contents);
	} else if (_type == MATLAB_STRUCT) {
	  SetStructEntry(row, col, contents);
	} else if (_type == MATLAB_MATRIX && contents.GetNumberOfElements() == 1) {
	  const int rows = mxGetM(_matrix);
	  if (mxIsDouble(_matrix)) {
	    ((double*) mxGetData(_matrix))[row + col * rows] = (double) contents.GetScalar();
	  } else if (mxIsSingle(_matrix)) {
	    ((float*) mxGetData(_matrix))[row + col * rows] = (float) contents.GetScalar();
	  }
	}

	return (*this);
      }

      // Print a friendly version of the matrix based on the type.
      friend std::ostream& operator<<(std::ostream& os, const MatlabMatrix& obj) {
	const Pair<int> dimensions = obj.GetDimensions();
	if (obj.GetMatrixType() == MATLAB_MATRIX) {
	  if (FLAGS_v >= 1) {
	    os << "Matrix Format:" << std::endl;
	  }
	  os << obj.GetCopiedContents();
	} else if (obj.GetMatrixType() == MATLAB_CELL_ARRAY) {
	  if (FLAGS_v >= 1) {
	    os << "Cell Format:" << std::endl;
	  }
	  for (int i = 0; i < dimensions.x; i++) {
	    for (int j = 0; j < dimensions.y; j++) {
	      os << "{" << i << "," << j << "}: " << std::endl << obj.GetCell(i, j) << std::endl;
	    }
	  }
	} else if (obj.GetMatrixType() == MATLAB_STRUCT) {
	  if (FLAGS_v >= 1) {
	    os << "Struct Format:" << std::endl;
	  }
	  const std::vector<std::string> fields = obj.GetStructFieldNames();
	  for (int i = 0; i < dimensions.x; i++) {
	    for (int j = 0; j < dimensions.y; j++) {
	      os << "(" << i << "," << j << "): " << std::endl;
	      for (int k = 0; k < fields.size(); k++) {
		os << fields[k] << ": " << obj.GetStructField(fields[k], i, j) << std::endl;
	      }
	      os << std::endl;
	    }
	  }
	} else if (obj.GetMatrixType() == MATLAB_STRING) {
	  os << "String Format: " << obj.GetStringContents();
	}
	os << std::endl;
	return os;
      }

      // Casting operator to std::vector type. This is a bit dangerous
      // to use as the type of the underlying MatlabMatrix is not
      // known at compile time, so static casting can still miss
      // potential runtime errors.
      template <typename T>
      operator std::vector<T>() {
	std::vector<T> result(size());

	if (_type == MATLAB_CELL_ARRAY) {
	  // TODO(sean): Implement me. This is a tricky implementation
	  // because we are potentially mixing compilation and runtime
	  // attributes.
	} else if (_type == MATLAB_STRUCT) {
	  // TODO(sean): Implement me
	} else if (_type == MATLAB_MATRIX) {
	  for (int i = 0; i < size(); i++) {
	    result[i] = static_cast<T>(GetMatrixEntry(i));
	  }
	}

	return result;
      }

    private:
      mxArray* _matrix;
      bool _shared;
      MatlabMatrixType _type;

      void Initialize(const MatlabMatrixType& type, const Pair<int>& dimensions);

      MatlabMatrix(const mxArray* data);
      MatlabMatrix(mxArray* data);

      void LoadMatrixFromFile(const std::string& filename, const bool& multivariable);
      MatlabMatrixType GetType(const mxArray* data) const;

      void AssignData(mxArray* data);

      friend class MatlabConverter;
      friend class MatlabFunction;
    };

    class MatlabConverter {
    public:
      // Determine whether indices (as appropriate) should be offset
      // for use in MATLAB.  Default is that this is enabled.
      //
      // TODO(sean): This should be probably be a parameter to all
      // routines to avoid people missing this setting.
      static void EnableMatlabOffset();
      static void DisableMatlabOffset();
      static inline int GetMatlabOffset() {
	return _matlab_offset;
      }

      static MatlabMatrix ConvertModelToMatrix(const slib::svm::Model& model);
      static MatlabMatrix ConvertMetadataToMatrix(const std::vector<slib::svm::DetectionMetadata>& metadata,
						  const bool& minimal = false);

      // Assumes the indices and clusters are specified in 0-based indexing.
      static MatlabMatrix 
      ConvertDetectionsToMatrixSimplified(const slib::svm::DetectionResultSet& detections,
					  const std::vector<int>& image_indices = std::vector<int>(0),
					  const std::vector<int>& assigned_clusters = std::vector<int>(0));

      static slib::svm::Detector ConvertMatrixToDetector(const MatlabMatrix& matrix);
      static MatlabMatrix ConvertDetectorToMatrix(const slib::svm::Detector& detector);

    private:
      static int _matlab_offset;
    };    

  }  // namespace util
}  // namespace slib

#endif
