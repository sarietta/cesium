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
  VLOG(1) << "****************************";
  for (map<string, MatlabMatrix>::const_iterator iter = variables.begin();
       iter != variables.end(); iter++) {
    VLOG(1) << "Found variable: " << (*iter).first;
    VLOG(1) << (*iter).second;
    VLOG(1) << "===================";
  }
  VLOG(1) << "****************************\n\n";
}

MatlabMatrix MakeCellMatrix(const float value) {
  MatlabMatrix matrix(slib::util::MATLAB_CELL_ARRAY, Pair<int>(1, 1));
  matrix.SetCell(0, MatlabMatrix(value));
  return matrix;
}

void TestFunction1(const JobDescription& job, JobOutput* output) {
  VLOG(1) << "\n\nTestFunction1";
  ShowVariables(job.variables);

  output->indices = job.indices;
  output->variables["output_variable1"] = MakeCellMatrix(1.0f);
  output->variables["output_variable2"] = MakeCellMatrix(2.0f);
}

void TestFunction2(const JobDescription& job, JobOutput* output) {
  VLOG(1) << "\n\nTestFunction2";
  ShowVariables(job.variables);
 
  output->indices = job.indices;
  output->variables["output_variable3"] = MakeCellMatrix(3.0f);
  output->variables["output_variable4"] = MakeCellMatrix(4.0f);
}

void TestFunction3(const JobDescription& job, JobOutput* output) {
  VLOG(1) << "\n\nTestFunction3";
  ShowVariables(job.variables);
 
  output->indices = job.indices;

  MatlabMatrix matrix(slib::util::MATLAB_CELL_ARRAY, Pair<int>(3, 1));
  if (job.indices[0] == 0) {
    matrix.SetCell(0, MatlabMatrix("CELL 0"));
  }
  if (job.indices[0] == 1) {
    matrix.SetCell(1, MatlabMatrix("CELL 1"));
  }
  if (job.indices[0] == 2) {
    matrix.SetCell(2, MatlabMatrix("CELL 2"));
  }
  output->variables["output"] = matrix;
}

bool TEST_MATLAB_MATRIX_EQUAL(const MatlabMatrix& A, const MatlabMatrix& B) {
  return (A.Serialize() == B.Serialize());
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
  CESIUM_REGISTER_COMMAND(TestFunction3);

  Cesium* instance = Cesium::GetInstance();
  if (instance->Start() == slib::mpi::CesiumMasterNode) {
    FLAGS_logtostderr = true;
#if 0
    {
      JobDescription job;
      job.command = "TestFunction1";
      job.variables["1:: input_variable1"] = MatlabMatrix("input 1");
      job.variables["1:: input_variable2"] = MatlabMatrix(FloatMatrix::Random(2,2));
      job.indices.push_back(0);

      JobOutput output;
      instance->ExecuteJob(job, &output);
      VLOG(1) << "\n\nOUTPUT :: TestFunction1";
      ShowVariables(output.variables);

      ASSERT_TRUE(TEST_MATLAB_MATRIX_EQUAL(output.variables["output_variable1"], MakeCellMatrix(1.0f)));
      ASSERT_TRUE(TEST_MATLAB_MATRIX_EQUAL(output.variables["output_variable2"], MakeCellMatrix(2.0f)));
    }

    {
      JobDescription job;
      job.command = "TestFunction2";
      job.variables["2:: input_variable1"] = MatlabMatrix("input 2");
      job.variables["2:: input_variable2"] = MatlabMatrix(FloatMatrix::Random(2,2));
      job.indices.push_back(0);

      JobOutput output;
      instance->ExecuteJob(job, &output);
      VLOG(1) << "\n\nOUTPUT :: TestFunction2";
      ShowVariables(output.variables);

      ASSERT_TRUE(TEST_MATLAB_MATRIX_EQUAL(output.variables["output_variable3"], MakeCellMatrix(3.0f)));
      ASSERT_TRUE(TEST_MATLAB_MATRIX_EQUAL(output.variables["output_variable4"], MakeCellMatrix(4.0f)));
    }
#endif
    {
      JobDescription job;
      job.command = "TestFunction3";
      job.variables["variable1"] = MatlabMatrix(1.0f);
      job.indices.push_back(0);
      job.indices.push_back(1);
      job.indices.push_back(2);

      FLAGS_cesium_partial_variable_chunk_size = 1;
      FLAGS_cesium_temporary_directory = "/tmp";
      FLAGS_cesium_intelligent_parameters = false;

      instance->SetBatchSize(1);
      job.SetVariableType("output", slib::mpi::PARTIAL_VARIABLE_COLS);

      JobOutput output;
      instance->ExecuteJob(job, &output);
      VLOG(1) << "\n\nOUTPUT :: TestFunction3";
      ShowVariables(output.variables);

      MatlabMatrix cell1 = MatlabMatrix::LoadFromFile("/tmp/output/0.mat");
      MatlabMatrix cell2 = MatlabMatrix::LoadFromFile("/tmp/output/1.mat");
      MatlabMatrix cell3 = MatlabMatrix::LoadFromFile("/tmp/output/2.mat");

      ASSERT_TRUE(TEST_MATLAB_MATRIX_EQUAL(cell1.GetCell(0), MatlabMatrix("CELL 0")));
      ASSERT_TRUE(TEST_MATLAB_MATRIX_EQUAL(cell2.GetCell(1), MatlabMatrix("CELL 1")));
      ASSERT_TRUE(TEST_MATLAB_MATRIX_EQUAL(cell3.GetCell(2), MatlabMatrix("CELL 2")));
    }

    instance->Finish();
  }

  MPI_Finalize();

  LOG(INFO) << "ALL TESTS PASSED";

  return 0;
}
