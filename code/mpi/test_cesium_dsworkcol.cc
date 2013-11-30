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

MatlabMatrix cached_variable;

bool TEST_MATLAB_MATRIX_EQUAL(const MatlabMatrix& A, const MatlabMatrix& B) {
  return (A.Serialize() == B.Serialize());
}

void TestFunction(const JobDescription& job, JobOutput* output) {
  output->indices = job.indices;
  const int index = job.indices[0];
  const FloatMatrix A = job.GetInputByName("A").GetCopiedContents();

  MatlabMatrix top_detections(slib::util::MATLAB_CELL_ARRAY, Pair<int>(1, index + 1));

  MatlabMatrix metadata(slib::util::MATLAB_STRUCT, 1, 1);
  metadata.SetStructField("field", MatlabMatrix("content"));

  MatlabMatrix top_detection(slib::util::MATLAB_STRUCT, Pair<int>(1, 1));
  top_detection.SetStructField("scores", MatlabMatrix(0.5f));
  top_detection.SetStructField("imgIds", MatlabMatrix(index));
  top_detection.SetStructField("meta", metadata);
  
  top_detections.SetCell(0, index, top_detection);

  output->variables["dswork"].Merge(top_detections);
}

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  MPI_Init(&argc, &argv);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  LOG(INFO) << "Joining the job as processor: " << rank;

  CESIUM_REGISTER_COMMAND(TestFunction);

  FLAGS_cesium_intelligent_parameters = false;
  FLAGS_cesium_working_directory = "/tmp/dsworkcol";

  Cesium* instance = Cesium::GetInstance();
  instance->SetBatchSize(1);
  if (instance->Start() == slib::mpi::CesiumMasterNode) {
    FLAGS_logtostderr = true;

    const FloatMatrix A = FloatMatrix::Random(10, 10);
    const MatlabMatrix large_matrix(A);

    {
      JobDescription job;
      job.command = "TestFunction";
      job.variables["A"] = large_matrix;

      job.indices.push_back(0);
      job.indices.push_back(1);

      JobOutput output;
      output.SetVariableType("dswork", slib::mpi::DSWORK_COLUMN);
      instance->ExecuteJob(job, &output);
    }

    instance->Finish();
  }

  MPI_Finalize();

  LOG(INFO) << "ALL TESTS PASSED";

  return 0;
}
