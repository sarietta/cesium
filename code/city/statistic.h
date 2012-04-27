#ifndef __SLIB_CITY_STATISTIC_H__
#define __SLIB_CITY_STATISTIC_H__

#include "../common/types.h"
#include "attribute.h"
#include <cppconn/resultset.h>
#include <glog/logging.h>
#include <string>
#include <vector>

namespace slib {
  namespace city {
    class CensusBlockStatistic : public CensusAttribute {
    public:
      DEFINE_CENSUS_ATTRIBUTE(CensusBlockStatistic);

      CensusBlockStatistic() {}
      virtual bool Initialize(const sql::ResultSet& record);

      static void Filter(std::vector<CensusBlockStatistic*>* populations);
    private:
    };

  }  // namespace city
}  // namespace slib

#endif
