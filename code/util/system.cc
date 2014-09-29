#include "system.h"

#include <glog/logging.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

using std::string;

namespace slib {
  namespace util {

    bool System::ExecuteSystemCommand(const string& cmd, string* result) {
      VLOG(2) << "System Command Executing: " << cmd;
      // If result is NULL then we can just use system.
      if (result == NULL) {
	return (system(cmd.c_str()) != -1);
      } else {
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
    }

    string System::GetEnvironmentVariable(const string& name) {
      // According to the documentation for getenv, the returned
      // should not be modified (i.e. not deleted).
      char* environment_variable = getenv(name.c_str());
      if (environment_variable == NULL) {
	return "";
      } else {
	return string(environment_variable);
      }
    }
    
  }  // namespace util
}  // namespace slib
