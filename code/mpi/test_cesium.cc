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
#include <util/matlab.h>
#include <vector>

using slib::mpi::Cesium;
using slib::mpi::JobDescription;
using slib::mpi::JobOutput;
using slib::util::MatlabMatrix;
using std::cin;
using std::map;
using std::string;
using std::vector;

void ShowVariables(const map<string, MatlabMatrix> variables) {
  LOG(INFO) << "****************************";
  for (map<string, MatlabMatrix>::const_iterator iter = variables.begin();
       iter != variables.end(); iter++) {
    LOG(INFO) << "Found variable: " << (*iter).first;
    LOG(INFO) << (*iter).second;
    LOG(INFO) << "===================";
  }
  LOG(INFO) << "****************************\n\n";
}

MatlabMatrix MakeCellMatrix(const float value) {
  MatlabMatrix matrix(slib::util::MATLAB_CELL_ARRAY, Pair<int>(1, 1));
  matrix.SetCell(0, MatlabMatrix(value));
  return matrix;
}

void TestFunction1(const JobDescription& job, JobOutput* output) {
  LOG(INFO) << "\n\nTestFunction1";
  ShowVariables(job.variables);

  output->indices = job.indices;
  output->variables["output_variable1"] = MakeCellMatrix(1.0f);
  output->variables["output_variable2"] = MakeCellMatrix(2.0f);
}

void TestFunction2(const JobDescription& job, JobOutput* output) {
  LOG(INFO) << "\n\nTestFunction2";
  ShowVariables(job.variables);
 
  output->indices = job.indices;
  output->variables["output_variable3"] = MakeCellMatrix(3.0f);
  output->variables["output_variable4"] = MakeCellMatrix(4.0f);
}

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  // Initialize random number generator.
  srand(128);

  MPI_Init(&argc, &argv);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  LOG(INFO) << "Joining the job as processor: " << rank;

  CESIUM_REGISTER_COMMAND(TestFunction1);
  CESIUM_REGISTER_COMMAND(TestFunction2);

  Cesium* instance = Cesium::GetInstance();
  if (instance->Start() == slib::mpi::CesiumMasterNode) {
    {
      JobDescription job;
      job.command = "TestFunction1";
      job.variables["1:: input_variable1"] = MatlabMatrix("input 1");
      job.variables["1:: input_variable2"] = MatlabMatrix(FloatMatrix::Random(2,2));
      job.indices.push_back(0);

      JobOutput output;
      instance->ExecuteJob(job, &output);
      LOG(INFO) << "\n\nOUTPUT :: TestFunction1";
      ShowVariables(output.variables);
    }

    {
      JobDescription job;
      job.command = "TestFunction2";
      job.variables["2:: input_variable1"] = MatlabMatrix("input 2");
      job.variables["2:: input_variable2"] = MatlabMatrix(FloatMatrix::Random(2,2));
      job.indices.push_back(0);

      JobOutput output;
      instance->ExecuteJob(job, &output);
      LOG(INFO) << "\n\nOUTPUT :: TestFunction2";
      ShowVariables(output.variables);
    }

    instance->Finish();
  }

  return 0;
}
