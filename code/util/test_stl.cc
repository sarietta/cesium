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

  FLAGS_logtostderr = true;
  
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
  
  LOG(INFO) << "================ RANGES =================";
  const vector<float> range_f = slib::util::Range<float>(0, 10);
  const vector<int> range_i = slib::util::Range<int>(0, 6);
  const vector<char> range_c = slib::util::Range<char>(65, 80);

  LOG(INFO) << "Float range (0, 10): " << slib::util::PrintVector(range_f);
  LOG(INFO) << "Integer range (0, 6): " << slib::util::PrintVector(range_i);
  LOG(INFO) << "Char range (65, 80): " << slib::util::PrintVector(range_c);

  return 0;
}
