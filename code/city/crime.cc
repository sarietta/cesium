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

    bool Crime::LoadFromFile(const string& filename, vector<Attribute*>* attributes) {
      ifstream filestr(filename.c_str());
      if (!filestr.good()) {
	LOG(WARNING) << "Could not open file: " << filename;
	return false;
      }
      string line;
      getline(filestr, line);  // Eat the header.
      while (filestr.good()) {
	getline(filestr, line);
	vector<string> fields = slib::StringUtils::Explode(";", line);
	if (fields.size() != 11) {
	  continue;
	}
	const int latitude = atoi(fields[9].c_str());
	const int longitude = atoi(fields[10].c_str());
	
	LatLon location(static_cast<double>(latitude) / 1e6, static_cast<double>(longitude) / 1e6);
	// FIX ME (use different weight)
	attributes->push_back(new Crime(location, 1.0));
      }
      filestr.close();
      
      return true;
    }

  }  // namespace city
}  // namespace slib
