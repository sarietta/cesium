#include "../common/types.h"
#include "../string/stringutils.h"
#include "../util/statistics.h"
#include "attribute.h"
#include <glog/logging.h>
#include "house.h"
#include <math.h>
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
      // Read in all of the lat/lon pairs.
      int zwillowId, latitude, longitude, price, sqft;
      if (sscanf(line.c_str(), "%d %d %d %d %d", &zwillowId, &latitude, &longitude, &price, &sqft) == 5) {
	if (sqft <= 0) {
	  return false;
	}
	LatLon location(static_cast<double>(latitude) / 1e6, static_cast<double>(longitude) / 1e6);       
	_location = location;
	_weight = ((double) price) / ((double) sqft);

	return true;
      } else {
	return false;
      }
    }

#if 1
    void House::Filter(vector<House*>* houses) {
      int32 N = houses->size();
      
      float* prices = new float[N];
      for (int32 i = 0; i < N; i++) {
	prices[i] = log((*houses)[i]->GetWeight());
      }

      const float mean = slib::util::Mean<float>(prices, N);
      const float var = sqrt(slib::util::Variance<float>(prices, N));

      int deleted = 0;
      for (int32 i = 0; i < N; i++) {
	if (prices[i] < mean - 2 * var || prices[i] > mean + 2 * var) {
	  houses->erase(houses->begin() + i - deleted);
	  deleted++;
	} 
      }
      delete prices;
    }
#else
    void House::Filter(vector<House*>* houses) {
      int32 N = houses->size();
      for (int32 i = 0; i < N; i++) {
	if ((*houses)[i]->GetWeight() < 80000) {
	  houses->erase(houses->begin() + i);
	  i--;
	  N = houses->size();
	}
	if ((*houses)[i]->GetWeight() > 1e6) {
	  houses->erase(houses->begin() + i);
	  i--;
	  N = houses->size();
	}
      }
    }
#endif
    //REGISTER_TYPE(House, Attribute);

  }  // namespace city
}  // namespace slib
