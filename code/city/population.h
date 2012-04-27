#ifndef __SLIB_CITY_POPULATION_H__
#define __SLIB_CITY_POPULATION_H__

#include "../common/types.h"
#include "attribute.h"
#include <cppconn/resultset.h>
#include <glog/logging.h>
#include <string>
#include <vector>

namespace slib {
  namespace city {
    class CensusBlockPopulation : public CensusAttribute {
    public:
      DEFINE_CENSUS_ATTRIBUTE(CensusBlockPopulation);

      CensusBlockPopulation() {}
      virtual bool Initialize(const sql::ResultSet& record);

      static void Filter(std::vector<CensusBlockPopulation*>* populations);
    private:
    };

  }  // namespace city
}  // namespace slib

#endif
