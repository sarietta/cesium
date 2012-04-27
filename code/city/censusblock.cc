#include "censusblock.h"
#include <cppconn/exception.h>
#include <iostream>
#include <sstream>

using std::istream;

namespace slib {
  namespace city {
        
    CensusBlock::CensusBlock(const std::string& blockId, const slib::ShapefilePolygon& polygon) 
      : _blockId(blockId)
      , _polygon(polygon) {}
    
    bool CensusBlock::Initialize(const sql::ResultSet& record) {
      _value = 1;
      try {
	_blockId = record.getString("blockid");
	
	istream* polygon_encoded = record.getBlob("polygon");
	(*polygon_encoded) >>= _polygon;
	
	_voting_district = record.getInt("voting_district");
	_zipcode = record.getInt("zip_code");
	_land_area = record.getInt("land_area");
	_water_area = record.getInt("water_area");
	
	_name = record.getString("name");
	
	const double internal_point_lat = record.getDouble("internal_point_lat");
	const double internal_point_lon = record.getDouble("internal_point_lon");
	_internal_point = LatLon(internal_point_lat, internal_point_lon);
      } catch (sql::SQLException e) {
	return false;
      }

      _block = this;

      return true;
    }

    bool CensusBlock::InitializeWithBlock(const sql::ResultSet& record) {
      return Initialize(record);
    }
    
    void CensusBlock::AddAttribute(const CensusAttribute& attribute) {
    }
    
  }  // namespace city
}  // namespace slib
