#ifndef __SLIB_CITY_POPULATION_H__
#define __SLIB_CITY_POPULATION_H__

#include "../common/types.h"
#include "attribute.h"
#include <string>
#include <vector>

namespace slib {
  namespace city {
    class CensusBlockPopulation : public CensusAttribute {
    public:
      CensusBlockPopulation() {}
      CensusBlockPopulation(const LatLon& location, const double& weight);

      virtual bool Initialize(const ShapefilePolygon& polygon, const std::string& record);

      static void Filter(std::vector<CensusBlockPopulation*>* populations);
    };
  }  // namespace city
}  // namespace slib

#endif
