#include "asianpopulation.h"
#include "blackpopulation.h"
#include "definer.h"
#include "dwellings.h"
#include "censusblock.h"
#include "population.h"
#include "statistic.h"
#include "whitepopulation.h"

namespace slib {
  namespace city {
    
    void AttributeDefiner::Define() {
      REGISTER_CENSUS_ATTRIBUTE(CensusBlock);
      REGISTER_CENSUS_ATTRIBUTE(CensusBlockPopulation);
      REGISTER_CENSUS_ATTRIBUTE(CensusBlockDwellings);
      REGISTER_CENSUS_ATTRIBUTE(CensusBlockStatistic);
      REGISTER_CENSUS_ATTRIBUTE(AsianPopulation);
      REGISTER_CENSUS_ATTRIBUTE(BlackPopulation);
      REGISTER_CENSUS_ATTRIBUTE(WhitePopulation);
    }
    
  }  // namespace city
}  // namespace slib
