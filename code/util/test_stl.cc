#include <common/types.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>
#include "random.h"
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include "stl.h"
#include <string>

using std::string;
using std::vector;

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  
  vector<float> values;
  for (int i = 0; i < 10; i++) {
    values.push_back(slib::util::Random::Uniform(0.0f, 1.0f));
  }

  for (int i = 0; i < (int) values.size(); i++) {
    LOG(INFO) << values[i] << " (" << i << ")";
  }

  vector<int> indices = slib::util::Sort(&values);

  LOG(INFO) << "================ SORTED =================";

  for (int i = 0; i < (int) indices.size(); i++) {
    LOG(INFO) << values[i] << " (" << indices[i] << ")";
  }
  
  return 0;
}
