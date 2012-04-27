#include "../common/types.h"
#include "attribute.h"
#include "censusblock.h"
#include <string>

using std::ifstream;
using std::string;

namespace slib {
  namespace city {

    REGISTER_CATEGORY(Attribute);

    Attribute::Attribute(const LatLon& location, const double& weight) {
      _location = location;
      _weight = weight;
    }

    CensusAttribute::CensusAttribute(const LatLon& location, const double& weight) 
      : Attribute(location, weight) { }

    const ShapefilePolygon CensusAttribute::GetBlockGeometry() const {
      return _block->GetPolygon();
    }
    
  }  // namespace city
}  // namespace slib
