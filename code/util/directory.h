#ifndef __SLIB_UTIL_DIRECTORY_H__
#define __SLIB_UTIL_DIRECTORY_H__

#include <string>
#include <vector>

namespace slib {
namespace util {
  class Directory {
  private:
  public:
    static std::vector<std::string> 
    GetDirectoryContents(const std::string directory, 
			 const std::string filter = "");
  };
}  // namespace util
}  // namespace slib

#endif
