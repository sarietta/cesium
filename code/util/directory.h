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
    GetDirectoryContents(const std::string& directory, 
			 const std::string& filter = "",
			 const bool& recurse = false);

    static std::vector<std::string> 
    GetDirectoryFolders(const std::string& directory, 
			const bool& recurse = false);
  };
}  // namespace util
}  // namespace slib

#endif
