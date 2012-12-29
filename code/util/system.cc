#include "system.h"

#include <glog/logging.h>
#include <stdio.h>
#include <string>

using std::string;

namespace slib {
  namespace util {

    bool System::ExecuteSystemCommand(const string& cmd, string* result) {
      VLOG(2) << "System Command Executing: " << cmd;
      FILE* pipe = popen(cmd.c_str(), "r");
      if (!pipe) {
	string error(strerror(errno));
	fprintf(stderr, "%s", error.c_str());
	LOG(ERROR) << error; 
	return false;
      }
      char buffer[128];
      string output = "";
      while(!feof(pipe)) {
	if(fgets(buffer, 128, pipe) != NULL) {
	  output += buffer;
	}
      }
      pclose(pipe);

      if (result != NULL) {
	result->swap(output);
      }

      return true;
    }
    
  }  // namespace util
}  // namespace slib
