#include "census.h"
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

using slib::util::DBFReader;
using std::string;

DEFINE_string(dbf_file, "", "Test DBF file.");

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  
  DBFReader reader(FLAGS_dbf_file);
  string line;
  reader.NextRecord(&line);
  LOG(INFO) << "Line: " << line;

  return 0;
}
