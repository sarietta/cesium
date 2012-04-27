#ifndef __SLIB_UTIL_CENSUS_H__
#define __SLIB_UTIL_CENSUS_H__

#include "../common/types.h"
#include <iostream>
#include <map>
#include <stdio.h>
#include <string>
#include <vector>

namespace slib {
  struct ShapefilePolygon {
    double bbox[4];
    int num_parts;
    int num_points;
    int* parts;
    Point2D* points;

    ShapefilePolygon() {
      bbox[0] = 0; bbox[1] = 0; bbox[2] = 0; bbox[3] = 0;
      num_parts = 0;
      num_points = 0;
      parts = NULL;
      points = NULL;
    }

    friend std::ostream& operator<<(std::ostream& out, const ShapefilePolygon& polygon) {
      if (polygon.num_points <= 0) {
	return out;
      }
      out << polygon.points[0];
      for (int i = 1; i < polygon.num_points; i++) {
	out << ", " << polygon.points[i];
      }
      return out;
    }

    // Binary write.
    friend std::ostream& operator<<=(std::ostream& out, const ShapefilePolygon& polygon) {
      if (polygon.num_points <= 0) {
	return out;
      }
      out.write((char*) polygon.bbox, sizeof(double) * 4);
      out.write((char*) &polygon.num_parts, sizeof(int));
      out.write((char*) &polygon.num_points, sizeof(int));
      out.write((char*) polygon.parts, sizeof(int) * polygon.num_parts);
      for (int i = 0; i < polygon.num_points; i++) {
	out <<= polygon.points[i];
      }
      return out;
    }

    // Binary read.
    friend std::istream& operator>>=(std::istream& in, ShapefilePolygon& polygon) {
      in.read((char*) polygon.bbox, sizeof(double) * 4);
      in.read((char*) &polygon.num_parts, sizeof(int));
      in.read((char*) &polygon.num_points, sizeof(int));
      if (polygon.num_parts > 0) {
	polygon.parts = new int[polygon.num_points];
	in.read((char*) polygon.parts, sizeof(int) * polygon.num_parts);
      }
      if (polygon.num_points > 0) {
	polygon.points = new Point2D[polygon.num_points];
	for (int i = 0; i < polygon.num_points; i++) {
	  in >>= polygon.points[i];
	}
      }
      return in;
    }
  };

  namespace util {

    class ShapefileReader {
    public:
      ShapefileReader(const std::string& filename);
      virtual ~ShapefileReader();

      bool HasNextRecord();
      bool NextRecord(ShapefilePolygon* polygon);
      inline int64 GetBytesRead() {
	return _bytesRead;
      }

      void Close();

    private:
      void ParseHeader();

      std::string _filename;
      FILE* _fid;
      int64 _bytesRead;
    };

    class DBFReader {
      struct DBFField {
	std::string name;
	int length;
      };

    public:
      DBFReader(const std::string& filename);
      virtual ~DBFReader();

      bool HasNextRecord();
      bool NextRecord(std::string* record);
      inline int64 GetBytesRead() {
	return _bytesRead;
      }
      inline std::vector<DBFField> GetFields() {
	return _fields;
      }

      void Close();

    private:
      void ParseHeader();

      std::string _filename;
      FILE* _fid;
      int _record_length;
      int64 _bytesRead;
      std::vector<DBFField> _fields;
    };

  }  // namespace util
}  // namespace slib

#endif
