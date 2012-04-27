#include "../common/types.h"
#include "../string/stringutils.h"
#include "attribute.h"
#include "censusblock.h"
#include "dwellings.h"
#include "../registration/registration.h"
#include <cppconn/exception.h>
#include <fstream>
#include <glog/logging.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

DECLARE_string(slib_city_table_field);

using std::ifstream;
using std::string;
using std::vector;

namespace slib {
  namespace city {

    bool CensusBlockDwellings::Initialize(const sql::ResultSet& record) {
      _block = NULL;
      try {
	if (FLAGS_slib_city_table_field != "") {
	  const int total_dwellings = record.getInt("dwellings");
	  if (FLAGS_slib_city_table_field == "dwellings") {
	    _value = total_dwellings;
	  } else if (total_dwellings == 0) {
	    _value = 0;
	  } else {
	    _value = ((double) record.getInt(FLAGS_slib_city_table_field)) 
	      / ((double) total_dwellings);
	  }
	} else {
	  _value = (double) record.getInt("dwellings");
	}
      } catch (sql::SQLException e) {
	return false;
      }

      return true;
    }

    void CensusBlockDwellings::Filter(vector<CensusBlockDwellings*>* populations) {}

  }  // namespace city
}  // namespace slib
