#ifndef __SLIB_CITY_CENSUS_BLOCK_H__
#define __SLIB_CITY_CENSUS_BLOCK_H__ 

#include "../util/census.h"
#include "attribute.h"
#include <cppconn/resultset.h>

namespace slib {
  namespace city {

    class CensusBlock : public CensusAttribute {
    public:
      CensusBlock() {}
      CensusBlock(const LatLon& location, const double& weight);
      CensusBlock(const std::string& blockId, const slib::ShapefilePolygon& polygon);
      virtual ~CensusBlock() {}
      
      virtual bool Initialize(const slib::ShapefilePolygon& polygon, 
			      const std::string& record) { 
	return false; 
      }

      virtual bool Initialize(const sql::ResultSet& record);

      void AddAttribute(const CensusAttribute& attribute);
      
      inline const ShapefilePolygon GetPolygon() const {
	return _polygon;
      }

    private:
      std::string _blockId;
      slib::ShapefilePolygon _polygon;
      int32 _voting_district;
      int32 _zipcode;
      int32 _land_area;
      int32 _water_area;
      std::string _name;
      LatLon _internal_point;
      
      std::vector<CensusAttribute> _attributes;
    };
    
  }  // namespace city
}  // namespace slib

#endif
