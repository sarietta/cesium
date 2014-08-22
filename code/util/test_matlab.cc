#include "assert.h"
#include <CImg.h>
#include <common/types.h>
#undef Success
#include <Eigen/Dense>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <map>
#include "matlab.h"
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

DEFINE_int32(iterations, 1, "Iterations to test memory usage");

DEFINE_bool(test_accessors, false, "");
DEFINE_bool(test_mutators, false, "");
DEFINE_bool(test_celltomatrix, false, "");

DEFINE_string(test_file, "", "Path to .mat file to be used in the test.");

using Eigen::MatrixXf;
using slib::util::MatlabMatrix;
using std::map;
using std::string;

bool TEST_MATLAB_MATRIX_EQUAL(const MatlabMatrix& A, const MatlabMatrix& B) {
  return (A.Serialize() == B.Serialize());
}

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  FLAGS_logtostderr = true;
  
  if (FLAGS_test_accessors) {
    if (FLAGS_test_file == "") {
      LOG(ERROR) << "You must specify a test .mat file to use. One is provided in ${SLIB_CODE_DIR}/util/test.mat";
      return 1;
    }

    for (int i = 0; i < FLAGS_iterations; i++) {
      MatlabMatrix matrix = MatlabMatrix::LoadFromFile(FLAGS_test_file);
      MatlabMatrix field1 = matrix.GetStructField("field1");
      MatlabMatrix field2 = matrix.GetStructField("field2");
      MatlabMatrix field3 = matrix.GetStructField("field3");
      
      MatlabMatrix subfield1 = field1.GetStructField("subfield1");
      MatlabMatrix subfield2 = field1.GetStructField("subfield2");
      
      MatlabMatrix subfield2_cell1 = subfield2.GetCell(0, 0);
      MatlabMatrix subfield2_cell2 = subfield2.GetCell(1, 0);
      
      MatlabMatrix field2_cell1 = field2.GetCell(0, 0);
      MatlabMatrix field2_cell2 = field2.GetCell(0, 1);
      MatlabMatrix field2_cell3 = field2.GetCell(1, 0);
      MatlabMatrix field2_cell4 = field2.GetCell(1, 1);
      
      if (FLAGS_iterations == 1) {
	LOG(INFO) << "\n" << subfield1.GetCopiedContents();
	LOG(INFO) << "\n" << field3.GetCopiedContents();
	
	LOG(INFO) << "\n" << subfield2_cell1.GetCopiedContents();
	LOG(INFO) << "\n" << subfield2_cell2.GetCopiedContents();
	
	LOG(INFO) << "\n" << field2_cell1.GetCopiedContents();
	LOG(INFO) << "\n" << field2_cell2.GetCopiedContents();
	LOG(INFO) << "\n" << field2_cell3.GetCopiedContents();
	LOG(INFO) << "\n" << field2_cell4.GetCopiedContents();
      }
      FloatMatrix m(2,2);
      m << 1, 2, 3, 4;
      
      subfield1.SetContents(m);
      field1.SetStructField("subfield1", subfield1);
      matrix.SetStructField("field1", field1);
      
      matrix.SaveToFile("./test_c.mat");
      
      string serialized = matrix.Serialize();
      MatlabMatrix deserialized_matrix;
      deserialized_matrix.Deserialize(serialized);
      deserialized_matrix.SaveToFile("./test_c_serialized.mat");
      
      FloatMatrix R1 = MatrixXf::Random(3, 3);
      FloatMatrix R2 = MatrixXf::Random(3, 3);
      FloatMatrix R3 = MatrixXf::Random(3, 3);
      
      MatlabMatrix cell1(slib::util::MATLAB_CELL_ARRAY, Pair<int>(1, 1));
      MatlabMatrix cell2(slib::util::MATLAB_CELL_ARRAY, Pair<int>(1, 2));
      MatlabMatrix cell3(slib::util::MATLAB_CELL_ARRAY, Pair<int>(1, 3));
      
      cell1.SetCell(0, MatlabMatrix(R1));
      cell2.SetCell(1, MatlabMatrix(R2));
      cell3.SetCell(2, MatlabMatrix(R3));
      
      MatlabMatrix cells(slib::util::MATLAB_CELL_ARRAY, Pair<int>(1, 1));
      cells.Merge(cell1).Merge(cell2).Merge(cell3);
      
      if (FLAGS_iterations == 1) {
	LOG(INFO) << "\nCell 1:\n" << cell1.GetCell(0).GetCopiedContents();
	LOG(INFO) << "\nCell 2:\n" << cell2.GetCell(1).GetCopiedContents();
	LOG(INFO) << "\nCell 3:\n" << cell3.GetCell(2).GetCopiedContents();
	LOG(INFO) << "\nCell 1:\n" << cells.GetCell(0).GetCopiedContents();
	LOG(INFO) << "\nCell 2:\n" << cells.GetCell(1).GetCopiedContents();
	LOG(INFO) << "\nCell 3:\n" << cells.GetCell(2).GetCopiedContents();
      }
      
      MatlabMatrix cells1(slib::util::MATLAB_CELL_ARRAY, Pair<int>(1, 4));
      cells1.SetCell(0, MatlabMatrix(R1));
      cells1.SetCell(2, MatlabMatrix(R2));
      cells1.SetCell(3, MatlabMatrix("hello from #4"));
      
      const string serialized_cells1 = cells1.Serialize();
      MatlabMatrix deserialized_cells1;
      deserialized_cells1.Deserialize(serialized_cells1);
      
      if (FLAGS_iterations == 1) {
	LOG(INFO) << "====================================";
	LOG(INFO) << "\nCell 1:\n" << cells1.GetCell(0).GetCopiedContents();
	LOG(INFO) << "\nCell 2:\n" << cells1.GetCell(1).GetCopiedContents();
	LOG(INFO) << "\nCell 3:\n" << cells1.GetCell(2).GetCopiedContents();
	LOG(INFO) << "\nCell 4:\n" << cells1.GetCell(3).GetStringContents();
	
	LOG(INFO) << "\nCell 1:\n" << deserialized_cells1.GetCell(0).GetCopiedContents();
	LOG(INFO) << "\nCell 2:\n" << deserialized_cells1.GetCell(1).GetCopiedContents();
	LOG(INFO) << "\nCell 3:\n" << deserialized_cells1.GetCell(2).GetCopiedContents();
	LOG(INFO) << "\nCell 4:\n" << deserialized_cells1.GetCell(3).GetStringContents();
      }
      
      MatlabMatrix struct1(slib::util::MATLAB_STRUCT, Pair<int>(1,2));
      MatlabMatrix struct2(slib::util::MATLAB_STRUCT, Pair<int>(1,1));
      
      struct1.SetStructField("field1", 0, MatlabMatrix(1.0f));
      struct2.SetStructField("field2", 0, MatlabMatrix(2.0f));
      
      struct1.SetStructEntry(1, struct2.GetCopiedStructEntry(0));
      struct1.SaveToFile("test_struct.mat");
      
      MatlabMatrix A(slib::util::MATLAB_MATRIX, Pair<int>(2, 2));
      LOG(INFO) << A.GetCopiedContents();
      A.Set(0, 0, MatlabMatrix(1.0f));
      A.Set(0, 1, MatlabMatrix(2.0f));
      A.Set(1, 0, MatlabMatrix(3.0f));
      A.Set(1, 1, MatlabMatrix(4.0f));
      LOG(INFO) << A.GetCopiedContents();
      
    }
  }

  if (FLAGS_test_mutators) {
    MatlabMatrix A(slib::util::MATLAB_CELL_ARRAY, Pair<int>(1,2));

    MatlabMatrix cell1(slib::util::MATLAB_STRUCT, Pair<int>(1,2));
    MatlabMatrix cell2(slib::util::MATLAB_STRUCT, Pair<int>(1,1));

    cell1.SetStructField("features", 0, MatlabMatrix(FloatMatrix::Random(1,10)));
    cell1.SetStructField("features", 1, MatlabMatrix(FloatMatrix::Random(1,10)));

    cell2.SetStructField("features", 0, MatlabMatrix(FloatMatrix::Random(1,10)));

    A.SetCell(0, cell1);
    A.SetCell(1, cell2);

    LOG(INFO) << "A:\n" 
	      << " {0}:\n" 
	      << "  (0):\n" 
	      << "   features: " << A.GetCell(0).GetStructField("features", 0).GetCopiedContents() << "\n"
	      << " {0}:\n" 
	      << "  (1):\n" 
	      << "   features: " << A.GetCell(0).GetStructField("features", 1).GetCopiedContents() << "\n"
	      << " {1}:\n" 
	      << "  (0):\n" 
	      << "   features: " << A.GetCell(1).GetStructField("features", 0).GetCopiedContents();

    MatlabMatrix cell1_m;
    A.GetMutableCell(0, &cell1_m);
    cell1_m.SetStructField("features", 0, MatlabMatrix(0.0f));
    cell1_m.SetStructField("features", 1, MatlabMatrix(10.0f));

    LOG(INFO) << "A:\n" 
	      << " {0}:\n" 
	      << "  (0):\n" 
	      << "   features: " << A.GetCell(0).GetStructField("features", 0).GetCopiedContents() << "\n"
	      << " {0}:\n" 
	      << "  (1):\n" 
	      << "   features: " << A.GetCell(0).GetStructField("features", 1).GetCopiedContents() << "\n"
	      << " {1}:\n" 
	      << "  (0):\n" 
	      << "   features: " << A.GetCell(1).GetStructField("features", 0).GetCopiedContents();

    map<string, MatlabMatrix> mapp;
    mapp["test"] = A;

    LOG(INFO) << "A:\n" 
	      << " {0}:\n" 
	      << "  (0):\n" 
	      << "   features: " << mapp["test"].GetCell(0).GetStructField("features", 0).GetCopiedContents() << "\n"
	      << " {0}:\n" 
	      << "  (1):\n" 
	      << "   features: " << mapp["test"].GetCell(0).GetStructField("features", 1).GetCopiedContents() << "\n"
	      << " {1}:\n" 
	      << "  (0):\n" 
	      << "   features: " << mapp["test"].GetCell(1).GetStructField("features", 0).GetCopiedContents();


    A = MatlabMatrix();
    mapp["test"] = MatlabMatrix();
  }

  if (FLAGS_test_celltomatrix) {
    LOG(INFO) << "Testing CellToMatrix";

    MatlabMatrix A(slib::util::MATLAB_CELL_ARRAY, 3, 2);
    A.SetCell(0, 0, MatlabMatrix((float[]){1.0f, 2.0f, 3.0f}, 1, 3));
    A.SetCell(0, 1, MatlabMatrix((float[]){4.0f, 5.0f, 6.0f}, 1, 3));
    A.SetCell(1, 0, MatlabMatrix((float[]){7.0f, 8.0f, 9.0f}, 1, 3));
    A.SetCell(1, 1, MatlabMatrix((float[]){10.0f, 11.0f, 12.0f}, 1, 3));
    A.SetCell(2, 0, MatlabMatrix());
    A.SetCell(2, 1, MatlabMatrix());

    MatlabMatrix golden((float[]){1.0f, 2.0f, 3.0f, 7.0f, 8.0f, 9.0f, 4.0f, 5.0f, 6.0f, 10.0f, 11.0f, 12.0f}, 4, 3);
    MatlabMatrix flattened = A.CellToMatrix();
    ASSERT_TRUE(TEST_MATLAB_MATRIX_EQUAL(golden, flattened));

    A.SetCell(0, 0, MatlabMatrix((float[]){1.0f, 2.0f}, 1, 2));
    flattened = A.CellToMatrix();
    ASSERT_TRUE(TEST_MATLAB_MATRIX_EQUAL(MatlabMatrix(), flattened));

    A.SetCell(0, 0, MatlabMatrix((float[]){1.0f, 2.0f, 3.0f}, 1, 3));

    MatlabMatrix B(slib::util::MATLAB_CELL_ARRAY, 3, 2);
    B.SetCell(0, 0, MatlabMatrix(slib::util::MATLAB_STRUCT, 1, 1).SetStructField("field", A.GetCell(0, 0)));
    B.SetCell(0, 1, MatlabMatrix(slib::util::MATLAB_STRUCT, 1, 1).SetStructField("field", A.GetCell(0, 1)));
    B.SetCell(1, 0, MatlabMatrix(slib::util::MATLAB_STRUCT, 1, 1).SetStructField("field", A.GetCell(1, 0)));
    B.SetCell(1, 1, MatlabMatrix(slib::util::MATLAB_STRUCT, 1, 1).SetStructField("field", A.GetCell(1, 1)));
    B.SetCell(2, 0, MatlabMatrix());
    B.SetCell(2, 1, MatlabMatrix());

    MatlabMatrix golden2(slib::util::MATLAB_STRUCT, 4, 1);
    golden2.SetStructField("field", 0, A.GetCell(0, 0));
    golden2.SetStructField("field", 1, A.GetCell(1, 0));
    golden2.SetStructField("field", 2, A.GetCell(0, 1));
    golden2.SetStructField("field", 3, A.GetCell(1, 1));

    flattened = B.CellToMatrix();
    ASSERT_TRUE(TEST_MATLAB_MATRIX_EQUAL(golden2, flattened));
  }

  LOG(INFO) << "All tests passed";
  
  return 0;
}
