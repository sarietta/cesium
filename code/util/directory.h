#ifndef __SLIB_UTIL_DIRECTORY_H__
#define __SLIB_UTIL_DIRECTORY_H__

#include <string>
#include <vector>

using std::string;
using std::vector;

namespace slib {
namespace util {
  class Directory {
  private:
  public:
    static vector<string> GetDirectoryContents(const string directory, 
					       const string filter = "");
  };
}  // namespace util
}  // namespace slib

#endif
