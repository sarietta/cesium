#include <common/types.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include "matlab.h"
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

DEFINE_int32(iterations, 1, "Iterations to test memory usage");

using slib::util::MatlabMatrix;
using std::string;

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  
  for (int i = 0; i < FLAGS_iterations; i++) {
    MatlabMatrix matrix("./test.mat");
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
      LOG(INFO) << "\n" << subfield1.GetContents();
      LOG(INFO) << "\n" << field3.GetContents();
      
      LOG(INFO) << "\n" << subfield2_cell1.GetContents();
      LOG(INFO) << "\n" << subfield2_cell2.GetContents();
      
      LOG(INFO) << "\n" << field2_cell1.GetContents();
      LOG(INFO) << "\n" << field2_cell2.GetContents();
      LOG(INFO) << "\n" << field2_cell3.GetContents();
      LOG(INFO) << "\n" << field2_cell4.GetContents();
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
  }
  
  return 0;
}
