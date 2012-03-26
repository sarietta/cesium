#include "../common/types.h"
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

    bool House::LoadFromFile(const string& filename, vector<Attribute*>* attributes) {
      char buffer[1024];
      FILE* fid = fopen(filename.c_str(), "r");
      if (!fid) {
	LOG(WARNING) << "Could not open file: " << filename;
	return false;
      }
      // Eat the first line.
      if (!fgets(buffer, 1024, fid)) {
	LOG(WARNING) << "Could not read header from file: " << filename;
	return false;
      }
      // Read in all of the lat/lon pairs.
      int zwillowId, latitude, longitude, price;
      while (fscanf(fid, "%d %d %d %d", &zwillowId, &latitude, &longitude, &price) != EOF) {
	LatLon location(static_cast<double>(latitude) / 1e6, static_cast<double>(longitude) / 1e6);       
	attributes->push_back(new House(location, static_cast<double>(price)));
      }
      fclose(fid);
      
      return true;
    }

  }  // namespace city
}  // namespace slib
