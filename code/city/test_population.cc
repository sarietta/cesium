#include <gflags/gflags.h>
#include <glog/logging.h>
#include "population.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "../util/assert.h"
#include <vector>

using slib::city::CensusAttributes;
using slib::city::CensusBlockPopulation;
using std::string;
using std::vector;

DEFINE_string(shapefile, "", "Test shape file.");

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  
  vector<CensusBlockPopulation*> population 
    = CensusAttributes<CensusBlockPopulation>(FLAGS_shapefile).GetAttributes();
  if (population.size() > 0) {
    LOG(INFO) << "Polygon: " << population[0]->GetBlockGeometry();
    LOG(INFO) << "Population: " << population[0]->GetWeight();
  } else {
    ASSERT_EQ(false, true);
  }

  return 0;
}
