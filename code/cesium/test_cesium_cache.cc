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
#include <vector>

using slib::mpi::Cesium;
using slib::mpi::JobDescription;
using slib::mpi::JobOutput;
using slib::util::Directory;
using slib::util::MatlabMatrix;
using std::cin;
using std::map;
using std::string;
using std::vector;

DEFINE_int32(matrix_size, 15000, "Number of rows/cols of the matrix to test.");

MatlabMatrix cached_variable;

bool TEST_MATLAB_MATRIX_EQUAL(const MatlabMatrix& A, const MatlabMatrix& B) {
  return (A.Serialize() == B.Serialize());
}

void TestFunction(const JobDescription& job, JobOutput* output) {
  output->indices = job.indices;

  if (job.indices[0] == 0) {
    cached_variable = job.GetInputByName("cached");
  } else {
    ASSERT_TRUE(TEST_MATLAB_MATRIX_EQUAL(job.GetInputByName("cached"), cached_variable));
  }
}

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  MPI_Init(&argc, &argv);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  LOG(INFO) << "Joining the job as processor: " << rank;

  CESIUM_REGISTER_COMMAND(TestFunction);

  Cesium* instance = Cesium::GetInstance();
  instance->DisableIntelligentParameters();
  instance->SetBatchSize(1);
  if (instance->Start() == slib::mpi::CesiumMasterNode) {
    FLAGS_logtostderr = true;

    const FloatMatrix A = FloatMatrix::Random(FLAGS_matrix_size, FLAGS_matrix_size);
    const MatlabMatrix large_matrix(A);

    {
      JobDescription job;
      job.command = "TestFunction";
      job.variables["cached"] = large_matrix;
      instance->SetVariableType("cached", large_matrix, slib::mpi::CACHED_VARIABLE);

      job.indices.push_back(0);
      job.indices.push_back(1);

      JobOutput output;
      instance->ExecuteJob(job, &output);
    }
  }

  LOG(INFO) << "ALL TESTS PASSED";

  return 0;
}
