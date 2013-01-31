#ifndef __SLIB_UTIL_MATLAB_H__
#define __SLIB_UTIL_MATLAB_H__

#include <CImg.h>
#include <common/scoped_ptr.h>
#include <common/types.h>
#undef Success
#include <Eigen/Dense>
#include <map>
#include <mat.h>
#include <iostream>
#include <string>
#include <sstream>

namespace slib {
  namespace svm {
    class Detector;
    class DetectionMetadata;
    class DetectionResultSet;
    class Model;
  }
}

namespace slib {
  namespace util {

    enum MatlabMatrixType {
      MATLAB_STRUCT, MATLAB_CELL_ARRAY, MATLAB_MATRIX, MATLAB_STRING, MATLAB_NO_TYPE
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

      static MatlabMatrix LoadFromFile(const std::string& filename);
      bool SaveToFile(const std::string& filename, const bool& struct_format = false) const;

      // TODO(sarietta): Slowly transition this to be GetMutable* and Get*.

      MatlabMatrix GetCopiedStructField(const std::string& field, const int& index = 0) const;
      // Gets the entire struct at the index. In MATLAB for a struct A, A(index).
      MatlabMatrix GetCopiedStructEntry(const int& index = 0) const;
      MatlabMatrix GetCopiedStructEntry(const int& row, const int& col) const;

      MatlabMatrix GetCopiedCell(const int& row, const int& col) const;
      MatlabMatrix GetCopiedCell(const int& index) const;
      FloatMatrix GetCopiedContents() const;

      // Non-mutator access. Should be faster and more memory efficient.
      const MatlabMatrix GetStructField(const std::string& field, const int& index = 0) const;
      const MatlabMatrix GetCell(const int& row, const int& col) const;
      const MatlabMatrix GetCell(const int& index) const;
      float GetMatrixEntry(const int& row, const int& col) const;
      const float* GetContents() const;

      // Mutable access. Use these at your own risk. You can seriously
      // corrupt the hierarchy of the matrices if you mess around.
      void GetMutableCell(const int& index, MatlabMatrix* cell) const;
      //MatlabMatrix* GetMutableStructField(const std::string& field, const int& index = 0) const;

      float GetScalar() const;
      std::string GetStringContents() const;

      void SetStructField(const std::string& field, const MatlabMatrix& contents);
      void SetStructField(const std::string& field, const int& index, const MatlabMatrix& contents);
      // Sets the entire struct at the specified index. 
      void SetStructEntry(const int& index, const MatlabMatrix& contents);
      void SetStructEntry(const int& row, const int& col, const MatlabMatrix& contents);

      void SetCell(const int& row, const int& col, const MatlabMatrix& contents);
      void SetCell(const int& index, const MatlabMatrix& contents);
      void SetContents(const FloatMatrix& contents);
      void SetStringContents(const std::string& contents);

      void SetScalar(const float& scalar);

      // Although the return type is a "string", the contents of that
      // string will be fwrite-style bytes.
      std::string Serialize() const;
      // The stream knows its own length, however this function is
      // recursive and needs to be able to start reading from the
      // correct position in the stream. A calling method does not
      // need to worry with these details... just use the default.
      int Deserialize(const std::string& str, const int& position = 0);

      Pair<int> GetDimensions() const;
      std::vector<std::string> GetStructFieldNames() const;
      
      inline int GetNumberOfElements() const {
	const Pair<int> dimensions = GetDimensions();
	return (dimensions.x * dimensions.y);
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

      inline void Set(const int& row, const int& col, const MatlabMatrix& contents) {
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
      }


    private:
      mxArray* _matrix;
      bool _shared;
      MatlabMatrixType _type;

      void Initialize(const MatlabMatrixType& type, const Pair<int>& dimensions);

      MatlabMatrix(const mxArray* data);
      MatlabMatrix(mxArray* data);

      void LoadMatrixFromFile(const std::string& filename);
      MatlabMatrixType GetType(const mxArray* data) const;

      void AssignData(mxArray* data);

      friend class MatlabConverter;
    };

    class MatlabConverter {
    public:
      static MatlabMatrix ConvertModelToMatrix(const slib::svm::Model& model);
      static MatlabMatrix ConvertMetadataToMatrix(const std::vector<slib::svm::DetectionMetadata>& metadata);

      // Assumes the indices and clusters are specified in 0-based indexing.
      static MatlabMatrix 
      ConvertDetectionsToMatrixSimplified(const slib::svm::DetectionResultSet& detections,
					  const std::vector<int>& image_indices = std::vector<int>(0),
					  const std::vector<int>& assigned_clusters = std::vector<int>(0));

      static slib::svm::Detector ConvertMatrixToDetector(const MatlabMatrix& matrix);
      static MatlabMatrix ConvertDetectorToMatrix(const slib::svm::Detector& detector);
    };

  }  // namespace util
}  // namespace slib

#endif
