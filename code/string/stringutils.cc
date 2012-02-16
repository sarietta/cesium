#include <string>
#include <vector>

#include "stringutils.h"

using std::string;
using std::vector;

namespace slib {

  string StringUtils::Replace(const string& needle, const string& haystack, const string& replace) {
    size_t uPos = 0; 
    size_t uFindLen = needle.length(); 
    size_t uReplaceLen = replace.length();
    string result = haystack;
    
    if( uFindLen != 0 ) {    
      for( ;(uPos = haystack.find( needle, uPos )) != string::npos; ) {
	result.replace(uPos, uFindLen, replace);
	uPos += uReplaceLen;
      }
    }
    return result;
  }
  
  vector<string> StringUtils::Explode(const string &delimiter, const string &str) {
    vector<string> arr;
    
    int strleng = str.length();
    int delleng = delimiter.length();
    if (delleng == 0)
      return arr;
    
    int i=0; 
    int k=0;
    while(i < strleng) {
      int j=0;
      while (i+j < strleng 
             && j < delleng 
             && str[i+j] == delimiter[j]) {
        j++;
      }
      if (j == delleng) {
        arr.push_back(str.substr(k, i-k));
        i += delleng;
        k = i;
      }
      else {
        i++;
      }
    }
    arr.push_back(str.substr(k, i-k));
    return arr;
  }


}  // namespace slib
