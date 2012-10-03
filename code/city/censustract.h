#ifndef __SLIB_CITY_CENSUS_TRACT_H__
#define __SLIB_CITY_CENSUS_TRACT_H__ 

#include "../util/census.h"
#include "attribute.h"
#include <cppconn/resultset.h>

namespace slib {
  namespace city {

    class CensusTract : public CensusAttribute {
    public:
      DEFINE_CENSUS_ATTRIBUTE(CensusTract);

      CensusTract() {}
      CensusTract(const std::string& tractId, const slib::ShapefilePolygon& polygon);
      virtual ~CensusTract() {}
      
      virtual bool Initialize(const slib::ShapefilePolygon& polygon, 
			      const std::string& record) { 
	return false; 
      }

      virtual bool Initialize(const sql::ResultSet& record);
      virtual bool InitializeWithTract(const sql::ResultSet& record);

      virtual const slib::ShapefilePolygon GetGeometry() const;

      void AddAttribute(const CensusAttribute& attribute);
      
      inline const ShapefilePolygon GetPolygon() const {
	return _polygon;
      }

    private:
      std::string _tractId;
      slib::ShapefilePolygon _polygon;
      int32 _land_area;
      int32 _water_area;
      std::string _name;
      LatLon _internal_point;
      
      std::vector<CensusAttribute> _attributes;
    };
    
  }  // namespace city
}  // namespace slib

#endif
