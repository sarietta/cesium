#include "../common/types.h"
#include "../string/stringutils.h"
#include "attribute.h"
#include "censusblock.h"
#include "statistic.h"
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
DECLARE_string(slib_city_normalization_field);

using std::ifstream;
using std::string;
using std::vector;

namespace slib {
  namespace city {

    bool CensusBlockStatistic::Initialize(const sql::ResultSet& record) {
      _block = NULL;
      _tract = NULL;
      _value = 0.0;
      try {
	if (FLAGS_slib_city_table_field != "") {
	  double total_count = 1;
	  if (FLAGS_slib_city_normalization_field != "") {
	    total_count = record.getDouble(FLAGS_slib_city_normalization_field);
	  }

	  if (total_count == 0) {
	    _value = 0;
	  } else {
	    _value = record.getDouble(FLAGS_slib_city_table_field) / total_count;
	  }
	}
      } catch (sql::SQLException e) {
	return false;
      }

      return true;
    }

    void CensusBlockStatistic::Filter(vector<CensusBlockStatistic*>* populations) {}

  }  // namespace city
}  // namespace slib
