#ifndef __SLIB_CITY_CRIME_H__
#define __SLIB_CITY_CRIME_H__

#include "../common/types.h"
#include <string>
#include <vector>

namespace slib {
  namespace city {
    class Attribute;
  }
}

namespace slib {
  namespace city {
    class Crime : public Attribute {
      Crime(const LatLon& location, const double& weight);
      static bool LoadFromFile(const std::string& filename, std::vector<Attribute*>* attributes);
    };
  }  // namespace city
}  // namespace slib

#endif
