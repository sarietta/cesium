#include "censustract.h"
#include <cppconn/exception.h>
#include <iostream>
#include <sstream>

using std::istream;

namespace slib {
  namespace city {
        
    CensusTract::CensusTract(const std::string& tractId, const slib::ShapefilePolygon& polygon) 
      : _tractId(tractId)
      , _polygon(polygon) {}
    
    bool CensusTract::Initialize(const sql::ResultSet& record) {
      _value = 1;
      try {
	_tractId = record.getString("tractid");
	
	istream* polygon_encoded = record.getBlob("polygon");
	(*polygon_encoded) >>= _polygon;
	
	_land_area = record.getInt("land_area");
	_water_area = record.getInt("water_area");
	
	_name = record.getString("name");
	
	const double internal_point_lat = record.getDouble("internal_point_lat");
	const double internal_point_lon = record.getDouble("internal_point_lon");
	_internal_point = LatLon(internal_point_lat, internal_point_lon);
      } catch (sql::SQLException e) {
	return false;
      }

      _block = NULL;
      _tract = NULL;

      return true;
    }

    const ShapefilePolygon CensusTract::GetGeometry() const {
      return _polygon;
    }

    bool CensusTract::InitializeWithTract(const sql::ResultSet& record) {
      return Initialize(record);
    }
    
    void CensusTract::AddAttribute(const CensusAttribute& attribute) {
    }
    
  }  // namespace city
}  // namespace slib
