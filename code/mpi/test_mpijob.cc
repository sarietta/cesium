#include <common/types.h>
#undef Success
#include <Eigen/Dense>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <map>
#include <mpi.h>
#include "mpijob.h"
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <util/matlab.h>

DEFINE_int32(matrix_size, 4, "");

using Eigen::MatrixXf;
using slib::mpi::JobController;
using slib::mpi::JobDescription;
using slib::util::MatlabMatrix;
using std::map;
using std::string;
using std::stringstream;

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  
  int rank, size;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  LOG(INFO) << "Processor " << rank << " reporting for duty";

  const int dim1 = FLAGS_matrix_size / 2;
  const int dim2 = FLAGS_matrix_size;

  if (rank == 0) {
    JobDescription job;
    job.command = "TEST COMMAND";
    job.indices.push_back(1);
    job.indices.push_back(2);
    job.indices.push_back(3);

    FloatMatrix contents = MatrixXf::Random(dim1,dim1);
    MatlabMatrix matrix(contents);

    FloatMatrix contents2 = MatrixXf::Random(dim2,dim2);
    MatlabMatrix matrix2(contents2);

    if (FLAGS_matrix_size < 10) {
      LOG(INFO) << "\n\tinput1:\n" << contents;
      LOG(INFO) << "\n\tinput2:\n" << contents2;
    } else {
      LOG(INFO) << "\n\tinput1";
      LOG(INFO) << "\n\tinput2";
    }

    job.variables["input1"] = matrix;
    job.variables["input2"] = matrix2;

    JobController controller;
    controller.StartJobOnNode(job, 1);
  } else {
    JobDescription job = slib::mpi::JobNode::WaitForJobData();
    LOG(INFO) << "Command: " << job.command;
    stringstream indices;
    for (uint32 i = 0; i < job.indices.size(); i++) {
      indices << i << "\t";
    }
    LOG(INFO) << "Indices: " << indices.str();
    for (map<string, MatlabMatrix>::const_iterator it = job.variables.begin(); 
	 it != job.variables.end(); 
	 it++) {
      const string name = (*it).first;
      const MatlabMatrix matrix = (*it).second;
      if (FLAGS_matrix_size < 10) {
	LOG(INFO) << "Input: \n\t" << name << ":\n" << matrix.GetContents();
      } else {
	LOG(INFO) << "Input: \n\t" << name;
      }
    }
  }

  MPI_Finalize();
  
  return 0;
}
