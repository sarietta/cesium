#ifndef __SLIB_CITY_HOUSE_H__
#define __SLIB_CITY_HOUSE_H__

#include "../common/types.h"
#include "../registration/registration.h"
#include "attribute.h"
#include <string>
#include <vector>

namespace slib {
  namespace city {
    class House : public Attribute {
    public:
      House() {}
      House(const LatLon& location, const double& weight);

      virtual bool InitializeFromLine(const std::string& line);

      static void Filter(std::vector<House*>* houses);
    };

  }  // namespace city
}  // namespace slib

#endif
