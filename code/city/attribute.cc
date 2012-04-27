#include "../common/types.h"
#include "attribute.h"
#include "censusblock.h"
#include <glog/logging.h>
#include <map>
#include <string>

DEFINE_string(hostname, "localhost", "The hostname where the mySQL database is located.");
DEFINE_string(database, "census", "The name of the database holding the training data.");
DEFINE_string(database_user, "silicon", "The user to use for connecting to the database.");
DEFINE_string(database_password, "0012259c5c", "The password to use for connecting to the database.");

DEFINE_string(slib_city_table_field, "", "The field to use for the population statistic.");

using std::cout;
using std::ifstream;
using std::map;
using std::pair;
using std::string;

namespace slib {
  namespace city {

    map<string, CensusAttribute*> CensusAttribute::_registry;

    Attribute::Attribute(const LatLon& location, const double& weight) {
      _location = location;
      _weight = weight;
    }

    const ShapefilePolygon CensusAttribute::GetBlockGeometry() const {
      if (_block) {
	return _block->GetPolygon();
      } else {
	ShapefilePolygon empty;
	return empty;
      }
    }

    CensusBlock* CensusAttribute::GetBlock() const {
      return _block;
    }

    CensusAttribute* CensusAttribute::CreateByName(const std::string& name) {
      map<string, CensusAttribute*>::iterator iter = CensusAttribute::_registry.find(name);
      if (iter == CensusAttribute::_registry.end()) {
	return NULL;
      }
      return (*iter).second->clone();
    }

    CensusAttribute* CensusAttribute::Register(const std::string& name, 
					       CensusAttribute* attribute) {
      attribute->clone();
      if (CensusAttribute::_registry.find(name) == CensusAttribute::_registry.end()) {
	pair<string, CensusAttribute*> entry(name, attribute);
	CensusAttribute::_registry.insert(entry);
      }
      return attribute;
    }

    bool CensusAttribute::InitializeWithBlock(const sql::ResultSet& record) {
      Initialize(record);
      _block = new CensusBlock();
      if (!_block->Initialize(record)) {
	return false;
      }

      return true;
    }

    
  }  // namespace city
}  // namespace slib
