#define SLIB_NO_DEFINE_64BIT
#define cimg_display 0

#include "cesium.h"

#include <common/scoped_ptr.h>
#include <common/types.h>
#undef Success
#include <Eigen/Dense>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>
#include <map>
#include <mpi.h>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <string/stringutils.h>
#include <util/assert.h>
#include <util/directory.h>
#include <util/matlab.h>
#include <util/stl.h>
#include <vector>

using slib::cesium::Cesium;
using slib::cesium::JobDescription;
using slib::cesium::JobOutput;
using slib::util::Directory;
using slib::util::MatlabMatrix;
using std::cin;
using std::map;
using std::string;
using std::vector;

typedef map<string, vector<int> >::const_iterator iterator;

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  MPI_Init(&argc, &argv);

  Cesium* instance = Cesium::GetInstance();
  if (instance->Start() == slib::cesium::CesiumMasterNode) {
    const map<string, vector<int> > hostnames = instance->GetHostnameNodes();
    for (iterator iter = hostnames.begin(); iter != hostnames.end(); ++iter) {
      LOG(INFO) << (*iter).first << ": " << slib::util::PrintVector((*iter).second);
    }

    const vector<string> nodes = instance->GetNodeHostnames();
    for (int i = 0; i < (int) nodes.size(); ++i) {
      LOG(INFO) << i << ": " << nodes[i];
    }
  }

  return 0;
}
