#include "../common/types.h"
#include "attribute.h"
#include <fstream>
#include <glog/logging.h>
#include <iostream>
#include <string>

using std::ifstream;
using std::string;

namespace slib {
  namespace city {

    Attributes::Attributes(const string& filename, const string& type) {
      ifstream filestr(filename.c_str());
      if (!filestr.good()) {
	LOG(WARNING) << "Could not open file: " << filename;
	return;
      }
      string line;
      while (filestr.good()) {
	getline(filestr, line);
	Attribute* attribute = AttributeRegistry::CreateByName(type);
	if (attribute) {
	  attribute->InitializeFromLine(line);
	  _attributes.push_back(attribute);
	}
      }
      filestr.close();
    }

  }  // namespace city
}  // namespace slib
