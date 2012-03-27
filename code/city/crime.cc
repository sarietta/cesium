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
      LOG(INFO) << "Crime";
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

    vector<Crime> Crime::LoadCrimesFromFile(const string& file) {
      vector<Crime> crimes;
  
      ifstream filestr(file.c_str());
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
	crimes.push_back(Crime(location, 1.0));
      }
      filestr.close();
      
      return crimes;
    }

    //REGISTER_TYPE(Crime, Attribute);

  }  // namespace city
}  // namespace slib
