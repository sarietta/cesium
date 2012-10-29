#ifndef __SLIB_CITY_WHITE_POPULATION_H__
#define __SLIB_CITY_WHITE_POPULATION_H__

#include "../common/types.h"
#include "attribute.h"
#include <cppconn/resultset.h>
#include <glog/logging.h>
#include <string>
#include <vector>

namespace slib {
  namespace city {
    class WhitePopulation : public CensusAttribute {
    public:
      DEFINE_CENSUS_ATTRIBUTE(WhitePopulation);

      WhitePopulation() {}
      virtual bool Initialize(const sql::ResultSet& record);

      static void Filter(std::vector<WhitePopulation*>* whitepopulations);
    private:
    };

  }  // namespace city
}  // namespace slib

#endif