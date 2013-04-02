#include "callback.h"

#include <common/types.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

using slib::common::Closure;
using std::string;
using std::vector;

void Increment(int* input) {
  (*input) = (*input) + 1;
}

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  
  int input = 3;
  LOG(INFO) << "Input: " << input;

  Closure* callback = NewCallback(&Increment, &input);
  callback->Run();
  LOG(INFO) << "Input: " << input;  

  return 0;
}
