#ifndef __SLIB_CITY_CRIME_H__
#define __SLIB_CITY_CRIME_H__

#include "../common/types.h"
#include "attribute.h"
#include <string>
#include <vector>

namespace slib {
  namespace city {
    class Crime : public Attribute {
    public:
      Crime() {}
      Crime(const LatLon& location, const double& weight);

      virtual bool InitializeFromLine(const std::string& line);

      static void Filter(std::vector<Crime*>* crimes);
    };
  }  // namespace city
}  // namespace slib

#endif
