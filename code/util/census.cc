#include "census.h"
#include <glog/logging.h>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string>

using std::map;
using std::string;

namespace slib {
  namespace util {
    
    ShapefileReader::ShapefileReader(const string& filename) 
      : _filename(filename) {
      _fid = fopen(filename.c_str(), "rb");
      if (!_fid) {
	LOG(WARNING) << "Could not open shapefile: " << filename;
      } else {
	ParseHeader();
      }
    }

    ShapefileReader::~ShapefileReader() {
      Close();
    }

    void ShapefileReader::Close() {
      if (_fid) {
	fclose(_fid);
	_fid = NULL;
      }
    }

    bool ShapefileReader::HasNextRecord() {
      if (!_fid) {
	return false;
      }
      return (feof(_fid) == 0);
    }

    bool ShapefileReader::NextRecord(ShapefilePolygon* polygon) {
      if (!_fid) {
	LOG(ERROR) << "File is closed: " << _filename;
	return false;
      }
      if (feof(_fid)) {
	Close();
	return false;
      }
      
      int bytes_read = 0;
      // Read in the record header.
      int record_number = -1;
      int record_length = -1;
      int shape_type = -1;
      bytes_read += fread(&record_number, sizeof(int), 1, _fid);
      bytes_read += fread(&record_length, sizeof(int), 1, _fid);
      bytes_read += fread(&shape_type, sizeof(int), 1, _fid);
      record_number = __builtin_bswap32(record_number);  // Endian swap
      record_length = __builtin_bswap32(record_length);  // Endian swap

      if (bytes_read != 3) {
	goto fail;
      }

      if (bytes_read == 3 && shape_type == 1) {
	polygon->num_parts = 0;
	polygon->num_points = 1;
	polygon->points = new Point2D[polygon->num_points];	
	if (fread(&(polygon->points[0].x), sizeof(double), 1, _fid) != 1 ||
	      fread(&(polygon->points[0].y), sizeof(double), 1, _fid) != 1) {
	  LOG(ERROR) << "Could not read polygon in record: " << record_number;
	  goto fail;
	}
      } else {
	if (bytes_read != 3 || shape_type != 5 /* Polygon */) {
	  LOG(ERROR) << "Shape is not a polygon: " << shape_type;
	  goto fail;
	}
	// Read in the first part of the polygon (no dynamic arrays).
	if (fread(polygon->bbox, sizeof(double), 4, _fid) != 4) {
	  LOG(ERROR) << "Could not read bounding box of polygon in record: " << record_number;
	  goto fail;
	}
	if (fread(&(polygon->num_parts), sizeof(int), 1, _fid) != 1
	    || polygon->num_parts < 0) {
	  LOG(ERROR) << "Could not read number of polygon parts in record: " << record_number;
	  goto fail;
	}
	if (fread(&(polygon->num_points), sizeof(int), 1, _fid) != 1
	    || polygon->num_points < 0) {
	  LOG(ERROR) << "Could not read number of polygon points in record: " << record_number;
	  goto fail;
	}
	polygon->parts = new int[polygon->num_parts];
	polygon->points = new Point2D[polygon->num_points];
	if (fread(polygon->parts, sizeof(int), polygon->num_parts, _fid) != (uint32) polygon->num_parts) {
	  LOG(ERROR) << "Could not read polygon parts in record: " << record_number;
	  goto fail;
	}
	for (int i = 0; i < polygon->num_points; i++) {
	  if (fread(&(polygon->points[i].x), sizeof(double), 1, _fid) != 1 ||
	      fread(&(polygon->points[i].y), sizeof(double), 1, _fid) != 1) {
	    LOG(ERROR) << "Could not read polygon points in record: " << record_number;
	    goto fail;
	  }
	}
      }

      _bytesRead += record_length*2 + 8;
      return true;

    fail:
      if (feof(_fid)) {
	Close();
	return false;
      } else {
	LOG(ERROR) << "Malformed record (" << record_number << ") in shapefile: " << _filename;
	return false;
      }
    }
        
    void ShapefileReader::ParseHeader() {
      // Really annoying changing endianness.
      int file_code = -1;  // BIG
      int unused[5];  // BIG
      int file_length = -1; // BIG
      int version = -1;
      int shape_type = -1;
      double mbr[4];  // Min bounding rect (xmin, ymin, xmax, ymax)
      double z_range[2];  // zmin, zmax
      double m_range[2];  // mmin, mmax
      
      if (fread(&file_code, sizeof(int), 1, _fid) != 1) {
	LOG(WARNING) << "Malformed shapefile: " << _filename;
	Close();
	return;
      }
      file_code = __builtin_bswap32(file_code);  // Endian swap
      
      if (fread(unused, sizeof(int), 5, _fid) != 5) {
	LOG(WARNING) << "Malformed header in shapefile: " << _filename;
	Close();
	return;
      }
      
      if (fread(&file_length, sizeof(int), 1, _fid) != 1) {
	LOG(WARNING) << "Malformed header in shapefile: " << _filename;
	Close();
	return;
      }
      file_length = __builtin_bswap32(file_length);  // Endian swap
      
      if (fread(&version, sizeof(int), 1, _fid) != 1) {
	LOG(WARNING) << "Malformed header in shapefile: " << _filename;
	Close();
	return;
      }
      if (fread(&shape_type, sizeof(int), 1, _fid) != 1) {
	LOG(WARNING) << "Malformed header in shapefile: " << _filename;
	Close();
	return;
      }
      if (fread(mbr, sizeof(double), 4, _fid) != 4) {
	LOG(WARNING) << "Malformed header in shapefile: " << _filename;
	Close();
	return;
      }
      if (fread(z_range, sizeof(double), 2, _fid) != 2) {
	LOG(WARNING) << "Malformed header in shapefile: " << _filename;
	Close();
	return;
      }
      if (fread(m_range, sizeof(double), 2, _fid) != 2) {
	LOG(WARNING) << "Malformed header in shapefile: " << _filename;
	Close();
	return;
      }
      
      VLOG(1) << "Header: " << file_code << " :: " 
	      << file_length << " :: " 
	      << version << " :: " 
	      << shape_type << " :: " 
	      << mbr[0] << " :: " << mbr[1] << " :: " << mbr[2] << " :: " << mbr[3] << " :: " 
	      << z_range[0] << " :: " << z_range[1] << " :: " 
	      << m_range[0] << " :: " << m_range[1];

      if (shape_type != 5 && shape_type != 1) {
	LOG(WARNING) << "Shape type reading for type " << shape_type << " is not implemented";
	Close();
	return;
      }

      _bytesRead = 100;
    }

    DBFReader::DBFReader(const string& filename) 
      : _filename(filename) {
      _fid = fopen(filename.c_str(), "rb");
      if (!_fid) {
	LOG(WARNING) << "Could not open dbf file: " << filename;
      } else {
	ParseHeader();
      }
    }

    DBFReader::~DBFReader() {
      Close();
    }

    void DBFReader::Close() {
      if (_fid) {
	fclose(_fid);
	_fid = NULL;
      }
    }

    bool DBFReader::HasNextRecord() {
      if (!_fid) {
	return false;
      }
      return (feof(_fid) == 0);
    }

    bool DBFReader::NextRecord(string* line) {
      if (!_fid) {
	LOG(ERROR) << "File is closed: " << _filename;
	return false;
      }
      if (feof(_fid)) {
	Close();
	return false;
      }

      char record[_record_length];
      if (fread(record, sizeof(char), _record_length, _fid) != (uint32) _record_length) {
	goto fail;
      }

      line->erase();
      line->append(string(record).substr(0, _record_length));

      _bytesRead += _record_length;

      return true;
    fail:
      if (feof(_fid)) {
	Close();
      } else {
	LOG(ERROR) << "Malformed record (near byte " << _bytesRead << ") in dbf file: " << _filename;
      }
      return false;
    }
        
    void DBFReader::ParseHeader() {
      char file_type;
      char last_update[3];  //YYMMDD
      int num_records;
      short first_record_offset;
      short record_length;
      int reserved[4];
      char table_flags;
      char code_page_mark;
      short reserved_null;

      if (fread(&file_type, sizeof(char), 1, _fid) != 1) {
	goto fail;
      }
      if (fread(last_update, sizeof(char), 3, _fid) != 3) {
	goto fail;
      }
      if (fread(&num_records, sizeof(int), 1, _fid) != 1) {
	goto fail;
      }
      if (fread(&first_record_offset, sizeof(short), 1, _fid) != 1) {
	goto fail;
      }
      if (fread(&record_length, sizeof(short), 1, _fid) != 1) {
	goto fail;
      }
      _record_length = (int) record_length;
      if (fread(reserved, sizeof(int), 4, _fid) != 4) {
	goto fail;
      }
      if (fread(&table_flags, sizeof(char), 1, _fid) != 1) {
	goto fail;
      }
      if (fread(&code_page_mark, sizeof(char), 1, _fid) != 1) {
	goto fail;
      }
      if (fread(&reserved_null, sizeof(short), 1, _fid) != 1) {
	goto fail;
      }
      _bytesRead = 32;

      // Read in the fields.
      char field_name[11];
      char field_type;
      int field_displacement;
      char field_length;
      char num_decimal_places;
      char field_flags;
      int next_value;
      char step_value;
      int field_reserved[2];
      int next_char;
      while ((next_char = fgetc(_fid)) != 0x0D) {
	ungetc(next_char, _fid);

	if (fread(field_name, sizeof(char), 11, _fid) != 11) {
	  goto fail;
	}
	if (fread(&field_type, sizeof(char), 1, _fid) != 1) {
	  goto fail;
	}
	if (fread(&field_displacement, sizeof(int), 1, _fid) != 1) {
	  goto fail;
	}
	if (fread(&field_length, sizeof(char), 1, _fid) != 1) {
	  goto fail;
	}
	if (fread(&num_decimal_places, sizeof(char), 1, _fid) != 1) {
	  goto fail;
	}
	if (fread(&field_flags, sizeof(char), 1, _fid) != 1) {
	  goto fail;
	}
	if (fread(&next_value, sizeof(int), 1, _fid) != 1) {
	  goto fail;
	}
	if (fread(&step_value, sizeof(char), 1, _fid) != 1) {
	  goto fail;
	}
	if (fread(field_reserved, sizeof(int), 2, _fid) != 2) {
	  goto fail;
	}

	LOG(INFO) << "Field: " << field_name << " :: " << ((int) field_length) << " :: " << field_type;

	DBFField field;
	field.name = string(field_name);
	field.length = (int) field_length;
	_fields.push_back(field);

	_bytesRead += 32;
      }


      return;

    fail:
      LOG(ERROR) << "Malformed header in dbf file: " << _filename;
      Close();
      return;
    }
    
  }  // namespace util
}  // namespace slib
