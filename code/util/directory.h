#ifndef __SLIB_UTIL_DIRECTORY_H__
#define __SLIB_UTIL_DIRECTORY_H__

#include <string>
#include <vector>

namespace slib {
namespace util {
  class Directory {
  private:
  public:
    static std::vector<std::string> 
    GetDirectoryContents(const std::string& directory, 
			 const std::string& filter = "",
			 const bool& recurse = false);

    static std::vector<std::string> 
    GetDirectoryFolders(const std::string& directory, 
			const bool& recurse = false);

    static bool Exists(const std::string& path);

    static std::string GenerateTemporaryFilename(const std::string& root = "./");
  };

  class File {
  public:
    // Determines if a file exists.
    static bool Exists(const std::string& path);
    
    // Get the directory from the path to the file.
    static std::string GetDirectory(const std::string& path);

    // Returns the contents of the file in a string.
    static std::string GetContentsAsString(const std::string& path);
  };
}  // namespace util
}  // namespace slib

#endif
