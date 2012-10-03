#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <vector>

#include "stringutils.h"

using std::string;
using std::vector;

namespace slib {
  
  string StringUtils::Replace(const string& needle, const string& haystack, const string& replace) {
    size_t uPos = 0; 
    size_t uFindLen = needle.length(); 
    size_t uReplaceLen = replace.length() == 0 ? 1 : replace.length();
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
  
  void StringUtils::Replace(const string& needle, vector<string>* strings, const string& replace) {
    if (!strings) {
      return;
    }
    for (int i = 0; i < (int) strings->size(); i++) {
      string replaced = StringUtils::Replace(needle, (*strings)[i], replace); 
      (*strings)[i].clear();
      (*strings)[i].append(replaced);
    }
  }
  
  // Copyright 2002 The RE2 Authors.  All Rights Reserved.
  // Use of this source code is governed by a BSD-style
  // license that can be found in the LICENSE file.
  void StringUtils::StringAppendV(string* dst, const char* format, va_list ap) {
    // First try with a small fixed size buffer
    char space[1024];
    
    // It's possible for methods that use a va_list to invalidate
    // the data in it upon use.  The fix is to make a copy
    // of the structure before using it and use that copy instead.
    va_list backup_ap;
    va_copy(backup_ap, ap);
    int result = vsnprintf(space, sizeof(space), format, backup_ap);
    va_end(backup_ap);
    
    if ((result >= 0) && (result < (int) sizeof(space))) {
      // It fit
      dst->append(space, result);
      return;
    }
    
    // Repeatedly increase buffer size until it fits
    int length = sizeof(space);
    while (true) {
      if (result < 0) {
	// Older behavior: just try doubling the buffer size
	length *= 2;
      } else {
	// We need exactly "result+1" characters
	length = result+1;
      }
      char* buf = new char[length];
      
      // Restore the va_list before we use it again
      va_copy(backup_ap, ap);
      result = vsnprintf(buf, length, format, backup_ap);
      va_end(backup_ap);
      
      if ((result >= 0) && (result < length)) {
	// It fit
	dst->append(buf, result);
	delete[] buf;
	return;
      }
      delete[] buf;
    }
  }
  
  string StringUtils::StringPrintf(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    string result;
    StringAppendV(&result, format, ap);
    va_end(ap);
    return result;
  }
  
  void StringUtils::SStringPrintf(string* dst, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    dst->clear();
    StringAppendV(dst, format, ap);
    va_end(ap);
  }
  
  void StringUtils::StringAppendF(string* dst, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    StringAppendV(dst, format, ap);
    va_end(ap);
  }
  
}  // namespace slib
