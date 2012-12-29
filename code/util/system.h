#ifndef __SLIB_UTIL_SYSTEM_H__
#define __SLIB_UTIL_SYSTEM_H__

#include <string>

namespace slib {
  namespace util {

    class System {
      // Executes the system command cmd and stores the response in the
      // result pointer. The method returns a boolean determining if the
      // command executed correctly NOT whether the result of that
      // execution was a "success".
      static bool ExecuteSystemCommand(const std::string& cmd, std::string* result = NULL);
    };

  }  // namespace util
}  // namespace slib

#endif
