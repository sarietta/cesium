#ifndef __SLIB_UTIL_SYSTEM_H__
#define __SLIB_UTIL_SYSTEM_H__

#include <string>

namespace slib {
  namespace util {

    class System {
    public:
      // Executes the system command cmd and stores the response in the
      // result pointer. The method returns a boolean determining if the
      // command executed correctly NOT whether the result of that
      // execution was a "success".
      static bool ExecuteSystemCommand(const std::string& cmd, std::string* result = NULL);

      // Returns the empty string if the variable is not found (equiv to bash).
      static std::string GetEnvironmentVariable(const std::string& name);
    };

  }  // namespace util
}  // namespace slib

#endif
