#ifndef __SLIB_CITY_ATTRIBUTE_H__
#define __SLIB_CITY_ATTRIBUTE_H__

#include "../common/types.h"
#include "../registration/registration.h"
#include <string>
#include <vector>

namespace slib {
  namespace city {

    class Attribute {
    public:
      Attribute() {}
      Attribute(const LatLon& location, const double& weight);
      virtual bool InitializeFromLine(const std::string& line) = 0;

      inline LatLon GetLocation() {
	return _location;
      }

      inline double GetWeight() {
	return _weight;
      }
    protected:
      LatLon _location;
      double _weight;
    };

    class Attributes {
    public:
      Attributes(const std::string& filename, const std::string& type);
      
      inline const std::vector<Attribute*> GetAttributes() {
	return _attributes;
      }

      inline const Attribute* GetAttribute(const int32& index) {
	return _attributes[index];
      }
    private:
      std::vector<Attribute*> _attributes;
    };

  }  // namespace city
}  // namespace slib

#endif
