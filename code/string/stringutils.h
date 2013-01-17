#ifndef __SLIB_STRING_STRINGUTILS_H__
#define __SLIB_STRING_STRINGUTILS_H__

#include <stdarg.h>
#include <string>
#include <sstream>
#include <vector>

using std::string;
using std::stringstream;
using std::vector;

namespace slib {
  class StringUtils {
  public:
    static string Replace(const string& needle, const string& haystack, 
			  const string& replace);
    static void Replace(const string& needle, vector<string>* strings, 
			const string& replace);
    static vector<string> Explode(const string& delimeter, const string& str);

    template <typename T>
    static string Implode(const vector<T>& values, const string& delimeter) {
      stringstream ss(stringstream::out);
      for (int i = 0; i < (int) values.size(); i++) {
	ss << values[i] << delimeter;
      }
      return ss.str();
    }

    // The following methods were copied from the RE2 library
    // (code.google.com/p/re2.
    static void StringAppendV(string* dst, const char* format, va_list ap);
    static string StringPrintf(const char* format, ...);
    static void SStringPrintf(string* dst, const char* format, ...);
    static void StringAppendF(string* dst, const char* format, ...);
  };
}  // namespace slib

#endif
