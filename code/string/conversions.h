#ifndef __SLIB_STRING_CONVERSIONS_H__
#define __SLIB_STRING_CONVERSIONS_H__

#include <string>

namespace slib {
  namespace string {
    inline bool ParseHexString(const std::string& hex, int* value) {
      *value = 0;
    
      int a = 0;
      int b = hex.length() - 1;
      for (; b >= 0; a++, b--) {
        if (hex[b] >= '0' && hex[b] <= '9') {
	  (*value) += (hex[b] - '0') * (1 << (a * 4));
        }
        else {
	  switch (hex[b]) {
	  case 'A':
	  case 'a':
	    (*value) += 10 * (1 << (a * 4));
	    break;
                    
	  case 'B':
	  case 'b':
	    (*value) += 11 * (1 << (a * 4));
	    break;
                    
	  case 'C':
	  case 'c':
	    (*value) += 12 * (1 << (a * 4));
	    break;
                    
	  case 'D':
	  case 'd':
	    (*value) += 13 * (1 << (a * 4));
	    break;
            
	  case 'E':
	  case 'e':
	    (*value) += 14 * (1 << (a * 4));
	    break;
            
	  case 'F':
	  case 'f':
	    (*value) += 15 * (1 << (a * 4));
	    break;
            
	  default:
	    return false;
	  }
        }
      }
      
      return true;
    }
  }  // namespace string
}  // namespace slib

#endif
