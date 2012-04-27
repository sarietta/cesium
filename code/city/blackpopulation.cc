#include "../common/types.h"
#include "../string/stringutils.h"
#include "attribute.h"
#include "censusblock.h"
#include "blackpopulation.h"
#include "../registration/registration.h"
#include <cppconn/exception.h>
#include <fstream>
#include <glog/logging.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

using std::ifstream;
using std::string;
using std::vector;

namespace slib {
  namespace city {

    bool BlackPopulation::Initialize(const sql::ResultSet& record) {
      _block = NULL;
      try {
	_value = (double) record.getInt("black_alone");
      } catch (sql::SQLException e) {
	return false;
      }

      return true;
    }

    void BlackPopulation::Filter(vector<BlackPopulation*>* populations) {}

  }  // namespace city
}  // namespace slib
