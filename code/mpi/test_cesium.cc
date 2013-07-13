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

MatlabMatrix LoadDetections(const string& directory, const string& features_filename);

float test_feature1[9] = {231.231, 2461.0351, 6713.13, 3515.3161, 718.1071, 1698.39, 15691.19, 1039.311, 31136.10};
float test_feature2[9] = {10.51, 52861.41, 12413.63, 33135.171, 366.1468, 2648.3923, 7667.023, 1361.34, 2136.36};
float test_feature3[9] = {23.242, 333.806, 30.4431, 923.538, 954.759, 6189.19, 7772.29, 685.647, 511.452};

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

MatlabMatrix MakeFeatureStruct(const float* feature) {
  MatlabMatrix matrix(slib::util::MATLAB_STRUCT, Pair<int>(1, 1));
  matrix.SetStructField("name", MatlabMatrix("Feature"));
  matrix.SetStructField("features", MatlabMatrix(feature, 1, 9));
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

void TestFunction3_1(const JobDescription& job, JobOutput* output) {
  VLOG(1) << "\n\nTestFunction3_1";
  ShowVariables(job.variables);
 
  output->indices = job.indices;

  MatlabMatrix matrix(slib::util::MATLAB_CELL_ARRAY, Pair<int>(3, 1));
  if (job.indices[0] == 0) {
    matrix.SetCell(0, MakeFeatureStruct(test_feature1));
  }
  if (job.indices[0] == 1) {
    matrix.SetCell(1, MakeFeatureStruct(test_feature2));
  }
  if (job.indices[0] == 2) {
    matrix.SetCell(2, MakeFeatureStruct(test_feature3));
  }
  output->variables["output"] = matrix;
}

void TestFunction3_2(const JobDescription& job, JobOutput* output) {
  VLOG(1) << "\n\nTestFunction3_2";
  ShowVariables(job.variables);

  output->indices = job.indices;
  output->variables = job.variables;
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
  CESIUM_REGISTER_COMMAND(TestFunction3_1);
  CESIUM_REGISTER_COMMAND(TestFunction3_2);

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
      job.command = "TestFunction3_1";
      job.variables["variable1"] = MatlabMatrix(1.0f);
      job.indices.push_back(0);
      job.indices.push_back(1);
      job.indices.push_back(2);

      FLAGS_cesium_partial_variable_chunk_size = 1;
      FLAGS_cesium_temporary_directory = "/tmp";
      FLAGS_cesium_intelligent_parameters = false;

      instance->SetBatchSize(1);

      JobOutput output;
      output.SetVariableType("output", slib::mpi::PARTIAL_VARIABLE_COLS);

      instance->ExecuteJob(job, &output);
      VLOG(1) << "\n\nOUTPUT :: TestFunction3_1";
      ShowVariables(output.variables);

      MatlabMatrix cell1 = MatlabMatrix::LoadFromFile("/tmp/output/0.mat");
      MatlabMatrix cell2 = MatlabMatrix::LoadFromFile("/tmp/output/1.mat");
      MatlabMatrix cell3 = MatlabMatrix::LoadFromFile("/tmp/output/2.mat");

      ASSERT_TRUE(TEST_MATLAB_MATRIX_EQUAL(cell1.GetCell(0), MakeFeatureStruct(test_feature1)));
      ASSERT_TRUE(TEST_MATLAB_MATRIX_EQUAL(cell2.GetCell(1), MakeFeatureStruct(test_feature2)));
      ASSERT_TRUE(TEST_MATLAB_MATRIX_EQUAL(cell3.GetCell(2), MakeFeatureStruct(test_feature3)));

      {
	MatlabMatrix partial_variable = LoadDetections("/tmp/output", "/tmp/output.features.bin");
	partial_variable.SaveToFile("/tmp/output.mat");
      }

      JobDescription job2;
      job2.command = "TestFunction3_2";

      FLAGS_cesium_working_directory = "/tmp";
      MatlabMatrix partial_variable = instance->LoadInputVariable("output", 
								  slib::mpi::FEATURE_STRIPPED_ROW_VARIABLE);
      job2.variables["output"] = partial_variable;
      job2.indices.push_back(0);
      job2.indices.push_back(1);
      job2.indices.push_back(2);

      JobOutput output2;
      instance->ExecuteJob(job2, &output2);
      
      VLOG(1) << "\n\nOUTPUT :: TestFunction3_2";
      ShowVariables(output2.variables);

      ASSERT_TRUE(TEST_MATLAB_MATRIX_EQUAL(output2.variables["output"].GetCell(0), 
					   MakeFeatureStruct(test_feature1)));
      ASSERT_TRUE(TEST_MATLAB_MATRIX_EQUAL(output2.variables["output"].GetCell(1), 
					   MakeFeatureStruct(test_feature2)));
      ASSERT_TRUE(TEST_MATLAB_MATRIX_EQUAL(output2.variables["output"].GetCell(2), 
					   MakeFeatureStruct(test_feature3)));
    }

    instance->Finish();
  }

  MPI_Finalize();

  LOG(INFO) << "ALL TESTS PASSED";

  return 0;
}

MatlabMatrix LoadDetections(const string& directory, const string& features_filename) {
  const vector<string> matfiles = Directory::GetDirectoryContents(directory, "mat");

  ASSERT_GT(matfiles.size(), 0);

  LOG(INFO) << "Number of partial inputs: " << matfiles.size();

  FILE* fid = fopen(features_filename.c_str(), "wb");
  if (!fid) {
    LOG(ERROR) << "Could not open file for writing: " << features_filename;
    return MatlabMatrix();
  }

  int feature_dimension = -1;
  int index = 0;
  MatlabMatrix merged_detections(slib::util::MATLAB_CELL_ARRAY, Pair<int>(1,1));
  for (int i = 0; i < (int) matfiles.size(); i++) {
    LOG(INFO) << "Processing partial input: " << matfiles[i];

    MatlabMatrix detections = MatlabMatrix::LoadFromFile(matfiles[i]);
    // Loop through the detections for each image.
    for (int j = 0; j < detections.GetNumberOfElements(); j++) {
      MatlabMatrix cell;
      detections.GetMutableCell(j, &cell);
      // Extract all of the features from each detection and replace
      // with an index.
      VLOG(2) << "Size of detections: " << j << "::" << cell.GetDimensions().x << " x " << cell.GetDimensions().y;
      for (int k = 0; k < cell.GetNumberOfElements(); k++) {
	const MatlabMatrix feature = cell.GetStructField("features", k);
	if (feature_dimension > 0) {
	  ASSERT_EQ(feature.GetNumberOfElements(), feature_dimension);
	} else {
	  feature_dimension = feature.GetNumberOfElements();
	}
	const float* data = feature.GetContents();

	// Save the feature to our feature file.
	fwrite(data, sizeof(float), feature_dimension, fid);

	// Replace the feature in the original file with a pointer to
	// the offset in the feature file.
	cell.SetStructField("features", k, MatlabMatrix((float) index));
	index++;
      }
    }
    fflush(fid);

    //detections.SaveToFile(matfiles[i]);
    merged_detections.Merge(detections);
  }  
  fclose(fid);

  LOG(INFO) << "Found a total of " << index << " features";

  return merged_detections;
}
