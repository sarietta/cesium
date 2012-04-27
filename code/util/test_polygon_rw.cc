#include "census.h"
#include "../common/types.h"
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

using slib::Point2D;
using slib::ShapefilePolygon;
using std::stringstream;

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  ShapefilePolygon polygon;

  polygon.num_parts = 1;
  polygon.parts = new int[1];
  polygon.parts[0] = 25;

  polygon.num_points = 2;
  polygon.points = new Point2D[2];
  polygon.points[0].x = 1;
  polygon.points[0].y = 0.1;
  polygon.points[1].x = 2;
  polygon.points[1].y = 0.2;

  stringstream polygon_encoded(stringstream::out | stringstream::in);
  polygon_encoded <<= polygon;

  ShapefilePolygon polygon_decoded;
  polygon_encoded >>= polygon_decoded;

  LOG(INFO) << polygon;
  LOG(INFO) << polygon_decoded;

  return 0;
}
