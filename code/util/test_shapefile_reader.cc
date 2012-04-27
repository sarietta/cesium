#include "census.h"
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <stdio.h>
#include <stdlib.h>

using slib::util::ShapefileReader;
using slib::ShapefilePolygon;

DEFINE_string(shapefile, "", "Test shapefile.");

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  
  ShapefileReader reader(FLAGS_shapefile);
  ShapefilePolygon polygon;
  reader.NextRecord(&polygon);
  LOG(INFO) << polygon;

  return 0;
}
