#include <CImg.h>
#include <common/types.h>
#undef Success
#include <Eigen/Dense>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <map>
#include <mpi.h>
#include "mpijob.h"
#include <iostream>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <util/matlab.h>

DEFINE_string(matrix_file, "", "If you want to see if a particular file can be sent, specify it here.");

using Eigen::MatrixXf;
using slib::mpi::JobController;
using slib::mpi::JobDescription;
using slib::mpi::JobNode;
using slib::mpi::JobOutput;
using slib::mpi::VariableType;
using slib::util::MatlabMatrix;
using std::map;
using std::string;
using std::stringstream;

bool abort_job = false;

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  int rank, size;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  LOG(INFO) << "Processor " << rank << " reporting for duty";

  if (rank == 0) {
    JobDescription job;
    job.command = "TEST COMMAND";

    // Tests the FloatMatrix
    FloatMatrix contents = MatrixXf::Random(3,3);
    MatlabMatrix matrix(contents);

    // Tests the cell matrix
    MatlabMatrix cell_matrix(slib::util::MATLAB_CELL_ARRAY, Pair<int>(2, 2));
    cell_matrix.SetCell(0, MatlabMatrix(0.0f));
    cell_matrix.SetCell(1, MatlabMatrix(1.0f));
    cell_matrix.SetCell(2, MatlabMatrix(2.0f));
    cell_matrix.SetCell(3, MatlabMatrix(3.0f));

    // Tests the struct matrix
    MatlabMatrix struct_matrix(slib::util::MATLAB_STRUCT, Pair<int>(1, 1));
    struct_matrix.SetStructField("field1", MatlabMatrix("value1"));
    struct_matrix.SetStructField("field2", MatlabMatrix("value2"));
    struct_matrix.SetStructField("field3", MatlabMatrix("value3"));

    // Tests the string matrix
    MatlabMatrix string_matrix(slib::util::MATLAB_STRING, Pair<int>(1,1));
    string_matrix.SetStringContents("!@#$%^&*()_+~QWERTYUIOP{}|ASDFGHJKL:\"ZXCVBNM<>?");

    // Test other file
    if (FLAGS_matrix_file != "") {
      MatlabMatrix other = MatlabMatrix::LoadFromFile(FLAGS_matrix_file);
      job.variables["other"] = other;
    }

    job.variables["matrix"] = matrix;
    job.variables["cell"] = cell_matrix;
    job.variables["struct"] = struct_matrix;
    job.variables["string"] = string_matrix;

    JobController controller;
    for (int i = 1; i < size; i++) {
      controller.StartJobOnNode(job, i);
    }
  } else {
    JobDescription job = slib::mpi::JobNode::WaitForJobData();
    LOG(INFO) << "Command: " << job.command;

    for (map<string, MatlabMatrix>::const_iterator it = job.variables.begin(); 
	 it != job.variables.end(); 
	 it++) {
      const string name = (*it).first;
      const MatlabMatrix matrix = (*it).second;
      if (name != "other") {
	LOG(INFO) << "Input: \n==============\n" << name << "\n==============\n" << matrix << "\n\n";
      } else {
	LOG(INFO) << "Input: \n==============\n" << name << "\n==============\n" << matrix.GetNumberOfElements() << "\n\n";
      }
    }
  }

  MPI_Finalize();
  
  return 0;
}
