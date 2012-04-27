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

using std::ifstream;
using std::string;
using std::vector;

namespace slib {
  namespace city {

    bool CensusBlockStatistic::Initialize(const sql::ResultSet& record) {
      _block = NULL;
      try {
	if (FLAGS_slib_city_table_field != "") {
	  _value = ((double) record.getInt(FLAGS_slib_city_table_field)) ;
	}
      } catch (sql::SQLException e) {
	return false;
      }

      return true;
    }

    void CensusBlockStatistic::Filter(vector<CensusBlockStatistic*>* populations) {}

  }  // namespace city
}  // namespace slib
