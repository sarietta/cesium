#ifndef __SLIB_STRING_STRINGUTILS_H__
#define __SLIB_STRING_STRINGUTILS_H__

#include <stdarg.h>
#include <string>
#include <sstream>
#include <vector>

using std::stringstream;
using std::vector;

namespace slib {
  class StringUtils {
  public:
    static std::string Replace(const std::string& needle, const std::string& haystack, 
			       const std::string& replace);
    static void Replace(const std::string& needle, vector<std::string>* strings, 
			const std::string& replace);
    static vector<std::string> Explode(const std::string& delimeter, const std::string& str);

    template <typename T>
    static std::string Implode(const vector<T>& values, const std::string& delimeter) {
      stringstream ss(stringstream::out);
      for (int i = 0; i < (int) values.size(); i++) {
	ss << values[i] << delimeter;
      }
      return ss.str();
    }

    // The following methods were copied from the RE2 library
    // (code.google.com/p/re2.
    static void StringAppendV(std::string* dst, const char* format, va_list ap);
    static std::string StringPrintf(const char* format, ...);
    static void SStringPrintf(std::string* dst, const char* format, ...);
    static void StringAppendF(std::string* dst, const char* format, ...);
  };
}  // namespace slib

#endif
