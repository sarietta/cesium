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

using slib::cesium::Cesium;
using slib::cesium::JobDescription;
using slib::cesium::JobOutput;
using slib::util::Directory;
using slib::util::MatlabMatrix;
using std::cin;
using std::map;
using std::string;
using std::vector;

bool TEST_MATLAB_MATRIX_EQUAL(const MatlabMatrix& A, const MatlabMatrix& B) {
  return (A.Serialize() == B.Serialize());
}

void TestFunction(const JobDescription& job, JobOutput* output) {
  MatlabMatrix A(slib::util::MATLAB_CELL_ARRAY, 14, 1);

  for (int i = 0; i < (int) job.indices.size(); i++) {
    A.SetCell(job.indices[i], 0, MatlabMatrix(job.indices[i]));

    output->indices.push_back(job.indices[i]);
  }
  output->variables["testmat"].Merge(A);
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
  if (instance->Start() == slib::cesium::CesiumMasterNode) {
    FLAGS_logtostderr = true;

    const FloatMatrix A = FloatMatrix::Random(10, 10);
    const MatlabMatrix large_matrix(A);

    {
      JobDescription job;
      job.command = "TestFunction";
      job.variables["cached"] = MatlabMatrix(A);

      for (int i = 0; i < 14; i++) {
	job.indices.push_back(i);
      }

      JobOutput output;
      instance->ExecuteJob(job, &output);
    }

    FLAGS_cesium_checkpointed_variables = "testmat";

    {
      JobDescription job;
      job.command = "TestFunction";
      job.variables["cached"] = MatlabMatrix(A);

      for (int i = 0; i < 14; i++) {
	job.indices.push_back(i);
      }

      JobOutput output;
      instance->ExecuteJob(job, &output);
    }

    instance->Finish();
  }

  MPI_Finalize();

  LOG(INFO) << "ALL TESTS PASSED";

  return 0;
}
