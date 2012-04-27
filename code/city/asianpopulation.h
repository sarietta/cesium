#ifndef __SLIB_CITY_ASIAN_POPULATION_H__
#define __SLIB_CITY_ASIAN_POPULATION_H__

#include "../common/types.h"
#include "attribute.h"
#include <cppconn/resultset.h>
#include <glog/logging.h>
#include <string>
#include <vector>

namespace slib {
  namespace city {
    class AsianPopulation : public CensusAttribute {
    public:
      DEFINE_CENSUS_ATTRIBUTE(AsianPopulation);

      AsianPopulation() {}
      virtual bool Initialize(const sql::ResultSet& record);

      static void Filter(std::vector<AsianPopulation*>* populations);
    private:
    };

  }  // namespace city
}  // namespace slib

#endif
