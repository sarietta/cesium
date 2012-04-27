#include "../common/types.h"
#include "../string/stringutils.h"
#include "attribute.h"
#include "crime.h"
#include <fstream>
#include <glog/logging.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

using std::ifstream;
using std::string;
using std::vector;

namespace slib {
  namespace city {

    Crime::Crime(const LatLon& location, const double& weight) : Attribute(location, weight) {}

    bool Crime::InitializeFromLine(const std::string& line) {
      vector<string> fields = slib::StringUtils::Explode(";", line);
      if (fields.size() != 11) {
	return false;
      }
      const int latitude = atoi(fields[9].c_str());
      const int longitude = atoi(fields[10].c_str());
      
      LatLon location(static_cast<double>(latitude) / 1e6, static_cast<double>(longitude) / 1e6);
      // FIX ME (use different weight)
      _location = location;
      _weight = 1.0;

      return true;
    }

    void Crime::Filter(vector<Crime*>* crimes) {}

    //REGISTER_TYPE(Crime, Attribute);

  }  // namespace city
}  // namespace slib
