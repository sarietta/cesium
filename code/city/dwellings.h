#ifndef __SLIB_CITY_DWELLINGS_H__
#define __SLIB_CITY_DWELLINGS_H__

#include "../common/types.h"
#include "attribute.h"
#include <cppconn/resultset.h>
#include <glog/logging.h>
#include <string>
#include <vector>

namespace slib {
  namespace city {
    class CensusBlockDwellings : public CensusAttribute {
    public:
      DEFINE_CENSUS_ATTRIBUTE(CensusBlockDwellings);

      CensusBlockDwellings() {}
      virtual bool Initialize(const sql::ResultSet& record);

      static void Filter(std::vector<CensusBlockDwellings*>* populations);
    private:
    };

  }  // namespace city
}  // namespace slib

#endif
