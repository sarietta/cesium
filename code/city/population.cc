#include "../common/types.h"
#include "../string/stringutils.h"
#include "attribute.h"
#include "censusblock.h"
#include "population.h"
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

    bool CensusBlockPopulation::Initialize(const sql::ResultSet& record) {
      _block = NULL;
      _tract = NULL;
      try {
	if (FLAGS_slib_city_table_field != "") {
	  const int total_population = record.getInt("total_count");
	  if (FLAGS_slib_city_table_field == "total_count") {
	    _value = total_population;
	  } else if (total_population == 0) {
	    _value = 0;
	  } else {
	    _value = ((double) record.getInt(FLAGS_slib_city_table_field)) 
	      / ((double) total_population);
	  }
	} else {
	  _value = (double) record.getInt("total_count");
	}
      } catch (sql::SQLException e) {
	return false;
      }

      return true;
    }

    void CensusBlockPopulation::Filter(vector<CensusBlockPopulation*>* populations) {}

  }  // namespace city
}  // namespace slib
