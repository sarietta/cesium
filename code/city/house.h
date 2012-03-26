#ifndef __SLIB_CITY_HOUSE_H__
#define __SLIB_CITY_HOUSE_H__

#include "../common/types.h"
#include "../common/registration/registration.h"
#include <string>
#include <vector>

namespace slib {
  namespace city {
    class Attribute;
  }
}

namespace slib {
  namespace city {
    class House : public Attribute {
      House(const LatLon& location, const double& weight);
      static bool LoadFromFile(const std::string& filename, std::vector<Attribute*>* attributes);

      REGISTER_TYPE(House, Attribute);
    };
  }  // namespace city
}  // namespace slib

#endif
