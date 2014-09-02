#define cimg_display 0

#include <cesium/cesium.h>
#include <common/scoped_ptr.h>
#include <common/types.h>
#include <Eigen/Dense>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <stdlib.h>
#include <stdio.h>
#include <util/matlab.h>
#include <util/stl.h>

using slib::cesium::Cesium;
using slib::cesium::JobDescription;
using slib::cesium::JobOutput;
using slib::util::MatlabMatrix;

// All functions should have this signature.
void TestFunction(const JobDescription& input, JobOutput* output) {
  // Get variable from the job input.
  const MatlabMatrix& foo = input.GetVariable("foo");

  // Get the job index.
  const int job_index = input.GetJobIndex();

  // Allocate an output matrix large enough to store an output at the
  // job index.
  output->variables["bar"].Assign(MatlabMatrix(slib::util::MATLAB_CELL_ARRAY, job_index + 1, 1));
  
  // Save some data to an output variable.
  output->variables["bar"].SetCell(job_index, MatlabMatrix(foo.GetMatrixEntry(job_index)));

  // Indicate that we have processed the index.
  output->indices.push_back(job_index);
} CESIUM_REGISTER_COMMAND(TestFunction);
// The above statement can be anywhere in any file that's linked to this binary.

int main(int argc, char** argv) {
  // Initialize Google libraries.
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  // Get a Cesium instance (singleton).
  Cesium* instance = Cesium::GetInstance();

  // Start Cesium. The Start() method returns CesiumMasterNode if the
  // node is the master, and causes the node to go into an infinite
  // listen loop otherwise.
  if (instance->Start() == slib::cesium::CesiumMasterNode) {
    JobDescription job;
    // This must match the name of the function.
    job.command = "TestFunction";

    // Some random test data.
    job.variables["foo"].Assign(FloatMatrix::Random(4, 1));

    // Process all of the elements of the test matrix.
    job.indices = slib::util::Range(0, 4);

    JobOutput output;
    instance->ExecuteJob(job, &output);

    LOG(INFO) << "Input: " << job.GetVariable("foo");
    LOG(INFO) << "Output: " << output.GetVariable("bar");
  }

  return 0;
}
