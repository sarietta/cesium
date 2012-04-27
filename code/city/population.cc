#include "../common/types.h"
#include "../string/stringutils.h"
#include "attribute.h"
#include "censusblock.h"
#include "population.h"
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

    CensusBlockPopulation::CensusBlockPopulation(const LatLon& location, const double& weight) 
      : CensusAttribute(location, weight) {}

    bool CensusBlockPopulation::Initialize(const ShapefilePolygon& polygon, const std::string& record) {
      string blockId = record.substr(16, 15).c_str();
      _block = new CensusBlock(blockId, polygon);
      _location = LatLon(0,0);
      if (record.length() > 50) {
	return false;
      }
      _weight = strtod(record.substr(41, 9).c_str(), NULL);

      return true;
    }

    void CensusBlockPopulation::Filter(vector<CensusBlockPopulation*>* populations) {}

    //REGISTER_TYPE(CensusBlockPopulation, Attribute);

  }  // namespace city
}  // namespace slib
