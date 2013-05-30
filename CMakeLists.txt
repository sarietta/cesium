cmake_minimum_required (VERSION 2.6)
project (slib)
# The version number.
set (slib_VERSION_MAJOR 0)
set (slib_VERSION_MINOR 12)

# Standard configuration parameters
find_program(INTEL_CC "icc")
if(INTEL_CC MATCHES INTEL_CC-NOTFOUND)
else(INTEL_CC MATCHES INTEL_CC-NOTFOUND)
  set(CMAKE_CXX_COMPILER "icc")
endif(INTEL_CC MATCHES INTEL_CC-NOTFOUND)

set(CMAKE_CXX_FLAGS "-O3 -Wall -std=c++0x")
add_definitions(-DSKIP_OPENCV)

include_directories(./thirdparty/CImg)

set(CMAKE_MODULE_PATH ./cmake)

# Required libraries
if(DEFINED ENV{EIGEN3_INCLUDE_DIR})
  include_directories($ENV{EIGEN3_INCLUDE_DIR})
else(DEFINED ENV{EIGEN3_INCLUDE_DIR})
  find_package(Eigen3)
  if(EIGEN3_FOUND)
    include_directories(${EIGEN3_INCLUDE_DIR})
  else(EIGEN3_FOUND)
    message(FATAL_ERROR "\nCouldn't automatically locate your Eigen3 installation directory. Please set the environment variable EIGEN3_INCLUDE_DIR to the correct directory.")
  endif(EIGEN3_FOUND)
endif(DEFINED ENV{EIGEN3_INCLUDE_DIR})

if(DEFINED ENV{MATLAB_INCLUDE_DIR})
else(DEFINED ENV{MATLAB_INCLUDE_DIR})
  message(FATAL_ERROR "\nPlease set the environment variable MATLAB_INCLUDE_DIR to the include directory of your MATLAB installation. This is typically something like /usr/local/MATLAB/R2011b/extern/include/.")
endif(DEFINED ENV{MATLAB_INCLUDE_DIR})
include_directories($ENV{MATLAB_INCLUDE_DIR})

set(MODULES util drawing string image gl interpolation svm mpi matrix kmeans)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ./lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ./lib)

# Compile each module
foreach(MODULE ${MODULES}) 

  file(GLOB ALL_FILES 
    "./code/${MODULE}/*.cc"
    "./code/${MODULE}/*.cpp"
    )

  set(FILES)
  foreach(FILE ${ALL_FILES})
    if(${FILE} MATCHES "test")
    else(${FILE} MATCHES "test")
      list(APPEND FILES ${FILE})
    endif(${FILE} MATCHES "test")
  endforeach(FILE)

  add_library(${MODULE} STATIC ${FILES})
  set_target_properties(${MODULE} PROPERTIES LINKER_LANGUAGE CXX)

endforeach(MODULE) 
