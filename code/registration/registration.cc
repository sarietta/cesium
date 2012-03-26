#include "registration.h"

#include <map>
#include <string>

using std::pair;
using std::map;
using std::string;

namespace slib {
  Registration::Registry* Registration::_registry;
  
  std::map<std::string, Pointer>* Registration::GetRegistry() {
    if (!_registry) {
      _registry = new Registry();
    }
    
    return _registry->Get();
  }

}  // namespace slib
