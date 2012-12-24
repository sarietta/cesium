#include <glob.h>
#include <glog/logging.h>
#include <string>
#include <sys/stat.h>
#include <vector>

#include <util/directory.h>

using std::string;
using std::vector;

namespace slib {
namespace util {

vector<string> Directory::GetDirectoryContents(const string& directory, 
					       const string& filter,
					       const bool& recurse) {
  vector<string> files;

  glob_t globs;
  string pattern = directory + "/*" + filter;
  int ret = glob(pattern.c_str(), 0, NULL, &globs);
  if (ret != 0) {
    switch (ret) {
    case GLOB_NOSPACE:
      LOG(ERROR) << "glob reported out of memory.";
    case GLOB_ABORTED:
      LOG(ERROR) << "glob failed for some unknown reason.";
    case GLOB_NOMATCH:
      return files;
    }
  }

  struct stat _stat;
  for (int i = 0; i < (int) globs.gl_pathc; i++) {
    char* file = globs.gl_pathv[i];
    stat(file, &_stat);
    
    if (S_ISDIR(_stat.st_mode)) { // if directory
      if (recurse) {
	const vector<string> files_ = GetDirectoryContents(file, filter, recurse);
	files.insert(files.begin(), files_.begin(), files_.end());
      } else {
	continue;
      }
    }
    
    files.push_back(string(file));
    VLOG(1) << "Found file: " << file;
  }
  return files;
}

}  // namespace util
}  // namespace slib
