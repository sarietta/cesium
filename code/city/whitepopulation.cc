#include "../common/types.h"
#include "../string/stringutils.h"
#include "attribute.h"
#include "censusblock.h"
#include "whitepopulation.h"
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

    bool WhitePopulation::Initialize(const sql::ResultSet& record) {
      _block = NULL;
      try {
	_value = (double) record.getInt("white_alone");
      } catch (sql::SQLException e) {
	return false;
      }

      return true;
    }

    void WhitePopulation::Filter(vector<WhitePopulation*>* populations) {}

  }  // namespace city
}  // namespace slib
