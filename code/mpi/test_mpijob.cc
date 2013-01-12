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

DEFINE_int32(matrix_size, 6, "");
DEFINE_bool(test_partial_variables, false, "");
DEFINE_bool(test_comm_error, false, "");

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

void HandleSEGV(int signo) {
  //MPI_Comm_call_errhandler(MPI_COMM_WORLD, MPI_ERR_OTHER);

  JobOutput output;
  output.command = "Aborted";
  VLOG(1) << "Sending completion message";
  JobNode::SendCompletionMessage(MPI_ROOT_NODE);
  VLOG(1) << "Waiting for completion response";
  JobNode::WaitForCompletionResponse(MPI_ROOT_NODE);
  LOG(INFO) << "Send job output to root";
  JobNode::SendJobDataToNode(output, MPI_ROOT_NODE);
  while(1) {
    sleep(1);
  }
}

void HandleJobCompleted(const JobOutput& output, const int& node) {
  LOG(INFO) << "Node " << node << " completed job: " << output.command;
  if (output.command == "Aborted") {
    LOG(INFO) << "Aborting";
    MPI_Abort(MPI_COMM_WORLD, 0);
    MPI_Finalize();
    LOG(INFO) << "Exiting";
    exit(1);
  }
}

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  
  if (signal(SIGSEGV, HandleSEGV) == SIG_ERR) {
    LOG(ERROR) << "Cannot handle signal: SEGV";
  }

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

    if (FLAGS_test_comm_error) {
      controller.SetCompletionHandler(&HandleJobCompleted);
      controller.StartJobOnNode(job, 1);
      while(1) {
	LOG(INFO) << "Checking for completion";
	sleep(5);
	controller.CheckForCompletion();
      }
    }

    if (FLAGS_test_partial_variables) {
      map<string, VariableType> variable_types;
      variable_types["input3"] = slib::mpi::PARTIAL_VARIABLE_ROWS;
      variable_types["input4"] = slib::mpi::PARTIAL_VARIABLE_COLS;
      
      MatlabMatrix input3(slib::util::MATLAB_CELL_ARRAY, Pair<int>(dim1, dim1));
      MatlabMatrix input4(slib::util::MATLAB_CELL_ARRAY, Pair<int>(dim2, dim2));
      for (int i = 0; i < dim1; i++) {
	for (int j = 0; j < dim1; j++) {
	  input3.SetCell(i, j, MatlabMatrix(contents(i, j)));
	}
      }
      for (int i = 0; i < dim2; i++) {
	for (int j = 0; j < dim2; j++) {
	  input4.SetCell(i, j, MatlabMatrix(contents2(i, j)));
	}
      }
      
      job.variables["input3"] = input3;
      job.variables["input4"] = input4;
      
      job.indices.clear();
      job.indices.push_back(0);
      job.indices.push_back(2);
      
      controller.StartJobOnNode(job, 1, variable_types);
    }
  } else {
    {
      JobDescription job = slib::mpi::JobNode::WaitForJobData();
      LOG(INFO) << "Command: " << job.command;
      stringstream indices;
      for (uint32 i = 0; i < job.indices.size(); i++) {
	indices << job.indices[i] << "\t";
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

    if (FLAGS_test_comm_error) {
      JobDescription job = slib::mpi::JobNode::WaitForJobData();
      LOG(INFO) << "Command: " << job.command;
      int* segfault = new int[10];
      const int fault = segfault[100000] * segfault[1000];

      LOG(ERROR) << "I shouldn't be here: " << fault;
    }

    if (FLAGS_test_partial_variables) {
      JobDescription job = slib::mpi::JobNode::WaitForJobData();
      LOG(INFO) << "Command: " << job.command;
      stringstream indices;
      for (uint32 i = 0; i < job.indices.size(); i++) {
	indices << job.indices[i] << "\t";
      }
      LOG(INFO) << "Indices: " << indices.str();
      for (map<string, MatlabMatrix>::const_iterator it = job.variables.begin(); 
	   it != job.variables.end(); 
	   it++) {
	const string name = (*it).first;
	const MatlabMatrix matrix = (*it).second;
	if (FLAGS_matrix_size < 10) {
	  if (matrix.GetMatrixType() == slib::util::MATLAB_MATRIX) {
	    LOG(INFO) << "Input: \n\t" << name << ":\n" << matrix.GetContents();
	  } else if (matrix.GetMatrixType() == slib::util::MATLAB_CELL_ARRAY) {
	    stringstream ss(stringstream::out);
	    ss << "Input: \n\t" << name << ":\n";
	    const Pair<int> dims = matrix.GetDimensions();
	    for (int row = 0; row < dims.x; row++) {
	      for (int col = 0; col < dims.y; col++) {
		ss << matrix.GetCell(row, col).GetContents() << " ";
	      }
	      ss << "\n";
	    }
	    LOG(INFO) << ss.str();
	  }
	} else {
	  LOG(INFO) << "Input: \n\t" << name;
	}
      }
    }
  }

  MPI_Finalize();
  
  return 0;
}
