#include "../common/types.h"
#include "../string/stringutils.h"
#include "attribute.h"
#include <glog/logging.h>
#include "house.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

using std::string;
using std::vector;

namespace slib {
  namespace city {

    House::House(const LatLon& location, const double& weight) : Attribute(location, weight) {}

    bool House::InitializeFromLine(const std::string& line) {
      LOG(INFO) << "House";
      // Read in all of the lat/lon pairs.
      int zwillowId, latitude, longitude, price;
      if (sscanf(line.c_str(), "%d %d %d %d", &zwillowId, &latitude, &longitude, &price) == 4) {
	LatLon location(static_cast<double>(latitude) / 1e6, static_cast<double>(longitude) / 1e6);       
	_location = location;
	_weight = price;

	return true;
      } else {
	return false;
      }
    }

    vector<House> House::LoadHousesFromFile(const string& file) {
      vector<House> houses;
      
      char buffer[1024];
      FILE* fid = fopen(file.c_str(), "r");
      // Eat the first line.
      fgets(buffer, 1024, fid);
      // Read in all of the lat/lon pairs.
      int zwillowId, latitude, longitude, price;
      while (fscanf(fid, "%d %d %d %d", &zwillowId, &latitude, &longitude, &price) != EOF) {
	LatLon location(static_cast<double>(latitude) / 1e6, static_cast<double>(longitude) / 1e6);
	houses.push_back(House(location, static_cast<double>(price)));
      }
      fclose(fid);
      
      return houses;
    }

    //REGISTER_TYPE(House, Attribute);

  }  // namespace city
}  // namespace slib
