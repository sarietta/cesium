#ifndef __SLIB_CITY_ATTRIBUTE_H__
#define __SLIB_CITY_ATTRIBUTE_H__

#include "../common/types.h"
#include "../registration/registration.h"
#include "../string/stringutils.h"
#include "../util/census.h"
#include <glog/logging.h>
#include <fstream>
#include <string>
#include <vector>

namespace slib {
  namespace city {
    class CensusBlock;
  }
}

namespace slib {
  namespace city {

    class Attribute {
    public:
      Attribute() {}
      Attribute(const LatLon& location, const double& weight);
      virtual bool InitializeFromLine(const std::string& line) = 0;

      inline LatLon GetLocation() const {
	return _location;
      }

      inline double GetWeight() const {
	return _weight;
      }

      inline void SetWeight(const double& weight) {
	_weight = weight;
      }

    protected:
      LatLon _location;
      double _weight;
    };

    class CensusAttribute : public Attribute {
    public:
      CensusAttribute() {}
      CensusAttribute(const LatLon& location, const double& weight);
      virtual bool InitializeFromLine(const std::string& line) {
	LOG(ERROR) << "Census data does not operate on ascii files directly";
	return false;
      }

      virtual bool Initialize(const slib::ShapefilePolygon& polygon, 
			      const std::string& record) = 0;

      const slib::ShapefilePolygon GetBlockGeometry() const;
    protected:
      CensusBlock* _block;
    };

    template <class T>
    class Attributes {
    public:
      Attributes() {}
      Attributes(const std::string& filename) {
	this->ReadAttributesFromFile(filename);
      }

      inline void Filter() {
	T::Filter(_attributes);
      }

      static inline void Filter(std::vector<T*>* attributes) {
	T::Filter(attributes);
      }
      
      inline const std::vector<T*> GetAttributes() const {
	return _attributes;
      }

      inline const T* GetAttribute(const int32& index) const {
	return _attributes[index];
      }

    protected:
      void ReadAttributesFromFile(const std::string& filename) {
	std::ifstream filestr(filename.c_str());
	if (!filestr.good()) {
	  LOG(WARNING) << "Could not open file: " << filename;
	  return;
	}
	std::string line;
	while (filestr.good()) {
	  getline(filestr, line);
	  T* attribute = new T;
	  if (attribute) {
	    if (attribute->InitializeFromLine(line)) {
	      _attributes.push_back(attribute);
	    }
	  } 
	}
	filestr.close();      
      }
      std::vector<T*> _attributes;
    };

    template<class T>
    class CensusAttributes : public Attributes<T> {
    public:
      CensusAttributes() {}
      CensusAttributes(const std::string& filename) {
	this->ReadAttributesFromFile(filename);
      }
    protected:
      void ReadAttributesFromFile(const std::string& shapefile) {
	const string dbf_file = slib::StringUtils::Replace("shp", shapefile, "dbf");
	slib::util::ShapefileReader reader(shapefile);
	slib::util::DBFReader dbf_reader(dbf_file);

	while (reader.HasNextRecord()) {
	  if (!dbf_reader.HasNextRecord()) {
	    LOG(ERROR) << "Shapefile and DBF file are out of sync";
	  }

	  ShapefilePolygon polygon;
	  std::string record;
	  if (reader.NextRecord(&polygon)) {
	    if (dbf_reader.NextRecord(&record)) {
	      T* attribute = new T;
	      if (attribute && attribute->Initialize(polygon, record)) {
		this->_attributes.push_back(attribute);
	      }
	    }
	  }
	}
      }
    };


  }  // namespace city
}  // namespace slib

#endif
