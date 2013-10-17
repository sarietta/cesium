#include "cesium.h"
#include <common/types.h>
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
#include <util/assert.h>
#include <util/matlab.h>

using slib::mpi::Cesium;
using slib::mpi::JobController;
using slib::mpi::JobDescription;
using slib::mpi::JobNode;
using slib::mpi::JobOutput;
using slib::mpi::VariableType;
using slib::util::MatlabMatrix;
using std::map;
using std::string;
using std::stringstream;

DEFINE_bool(use_handler, false, "If true, will use the error handler below.");

void TestFunction(const JobDescription& job, JobOutput* output) {
  output->indices.push_back(job.indices[0]);
}

void HandleError(const int& error_code, const int& node) {
  LOG(INFO) << "There was an error communicating with node: " << node;
}

namespace slib {
  namespace mpi {
    class TestCesiumCommunication {
    public:
      void Run() {
	Cesium* instance = Cesium::GetInstance();
	if (instance->Start() == slib::mpi::CesiumMasterNode) {
	  // Inject error by tricking the framework into thinking
	  // there are more nodes than there are.
	  instance->_size = instance->_size + 1;
	  FLAGS_logtostderr = true;
	  
	  JobDescription job;
	  job.command = "TestFunction";
	  job.variables["variable"] = MatlabMatrix("1");
	  job.indices.push_back(0);
	  job.indices.push_back(1);
	  job.indices.push_back(2);
	  
	  JobOutput output;
	  instance->ExecuteJob(job, &output);
	} 
	instance->Finish();
      }
    };
  }
}

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
    job.indices.push_back(1);

    JobController controller;
    if (FLAGS_use_handler) {
      controller.SetCommunicationErrorHandler(&HandleError);
    }

    // Purposefully cause a communication error.
    controller.StartJobOnNode(job, size);
  }

  MPI_Barrier(MPI_COMM_WORLD);

  CESIUM_REGISTER_COMMAND(TestFunction);
  slib::mpi::TestCesiumCommunication tester;
  tester.Run();

  MPI_Finalize();
  
  return 0;
}
