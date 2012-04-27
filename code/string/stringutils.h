#ifndef __SLIB_STRING_STRINGUTILS_H__
#define __SLIB_STRING_STRINGUTILS_H__

#include <string>
#include <vector>

using std::string;
using std::vector;

namespace slib {
  class StringUtils {
  public:
    static string Replace(const string& needle, const string& haystack, const string& replace);
    static void Replace(const string& needle, vector<string>* strings, const string& replace);
    static vector<string> Explode(const string& delimeter, const string& str);
  };
}  // namespace slib

#endif
