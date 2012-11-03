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
#include <svm/detector.h>
#include <svm/model.h>

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
      explicit MatlabMatrix(const std::string& contents);

      explicit MatlabMatrix(const float& data);
      MatlabMatrix(const float* conents, const int& rows, const int& cols);
      explicit MatlabMatrix(const FloatMatrix& contents);

      // Very important for STL compatibility.
      const MatlabMatrix& operator=(const MatlabMatrix& right);

      // This is a merge, but it is rather strict. It requires both
      // matrices to be the same type and the same size. In the event
      // that both matrices define the same index / field, the "this"
      // matrix will be given priority.
      MatlabMatrix& Merge(const MatlabMatrix& other);

      static MatlabMatrix LoadFromFile(const std::string& filename);
      bool SaveToFile(const std::string& filename, const std::string& variable_name = "data") const;

      MatlabMatrix GetStructField(const std::string& field, const int& index = 0) const;
      MatlabMatrix GetCell(const int& row, const int& col) const;
      MatlabMatrix GetCell(const int& index) const;
      FloatMatrix GetContents() const;
      std::string GetStringContents() const;

      void SetStructField(const std::string& field, const MatlabMatrix& contents);
      void SetStructField(const std::string& field, const int& index, const MatlabMatrix& contents);
      void SetCell(const int& row, const int& col, const MatlabMatrix& contents);
      void SetCell(const int& index, const MatlabMatrix& contents);
      void SetContents(const FloatMatrix& contents);
      void SetStringContents(const std::string& contents);

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

      inline MatlabMatrixType GetMatrixType() const {
	return _type;
      }

    private:
      mxArray* _matrix;
      MatlabMatrixType _type;

      void Initialize(const MatlabMatrixType& type, const Pair<int>& dimensions);

      MatlabMatrix(const mxArray* data);

      void LoadMatrixFromFile(const std::string& filename);
      MatlabMatrixType GetType(const mxArray* data) const;
    };

    class MatlabConverter {
    public:
      static MatlabMatrix ConvertModelToMatrix(const slib::svm::Model& model);
      static MatlabMatrix ConvertMetadataToMatrix(const std::vector<slib::svm::DetectionMetadata>& metadata);
    };

  }  // namespace util
}  // namespace slib

#endif
