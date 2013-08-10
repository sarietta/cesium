cmake_minimum_required (VERSION 2.6)
project (slib)
# The version number.
set (slib_VERSION_MAJOR 0)
set (slib_VERSION_MINOR 12)

# Standard configuration parameters
find_program(INTEL_CC "icc")
if(INTEL_CC MATCHES INTEL_CC-NOTFOUND)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
else(INTEL_CC MATCHES INTEL_CC-NOTFOUND)
  set(CMAKE_CXX_COMPILER "icc")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w1 -wd2536 -wd780 -mkl")
endif(INTEL_CC MATCHES INTEL_CC-NOTFOUND)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++0x")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -std=c++0x")
add_definitions(-DSKIP_OPENCV)

include_directories(./thirdparty/CImg)
include_directories(./code)

set(CMAKE_MODULE_PATH ./cmake)

# Required libraries

## Boost
find_package(Boost COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

## X11 (not required)
find_package(X11)
if (X11_FOUND)
  include_directories(${X11_INCLUDE_DIR})
else ()
  set_property (GLOBAL PROPERTY COMPILE_DEFINITIONS cimg_display=0)
endif ()

## Eigen3
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

## MATLAB
if(DEFINED ENV{MATLAB_INCLUDE_DIR})
else(DEFINED ENV{MATLAB_INCLUDE_DIR})
  message(FATAL_ERROR "\nPlease set the environment variable MATLAB_INCLUDE_DIR to the include directory of your MATLAB installation. This is typically something like /usr/local/MATLAB/R2011b/extern/include/.")
endif(DEFINED ENV{MATLAB_INCLUDE_DIR})
include_directories($ENV{MATLAB_INCLUDE_DIR})

## Google Flags
if(DEFINED ENV{GFLAGS_INCLUDE_DIR})
  include_directories($ENV{GFLAGS_INCLUDE_DIR})
else(DEFINED ENV{GFLAGS_INCLUDE_DIR})
  find_package(GoogleFlags)
  if(GFLAGS_FOUND)
    include_directories(${GFLAGS_INCLUDE_DIR})
  else(GFLAGS_FOUND)
    message(FATAL_ERROR "\nCouldn't automatically locate your Google Flags installation directory. Please set the environment variable GFLAGS_INCLUDE_DIR to the correct directory.")
  endif(GFLAGS_FOUND)
endif(DEFINED ENV{GFLAGS_INCLUDE_DIR})

## Google Logging
if(DEFINED ENV{GLOG_INCLUDE_DIR})
  include_directories($ENV{GLOG_INCLUDE_DIR})
else(DEFINED ENV{GLOG_INCLUDE_DIR})
  find_package(GoogleLogging)
  if(GLOG_FOUND)
    include_directories(${GLOG_INCLUDE_DIR})
  else(GLOG_FOUND)
    message(FATAL_ERROR "\nCouldn't automatically locate your Google Logging installation directory. Please set the environment variable GLOG_INCLUDE_DIR to the correct directory.")
  endif(GLOG_FOUND)
endif(DEFINED ENV{GLOG_INCLUDE_DIR})

## FFTW3
find_package(FFTW3 REQUIRED)
include_directories(${FFTW3_INCLUDE_DIRS})

## BLAS
#if(INTEL_CC MATCHES INTEL_CC-NOTFOUND)
#  find_package(BLAS REQUIRED)
#  include_directories(${BLAS_INCLUDE_DIRS})
#endif(INTEL_CC MATCHES INTEL_CC-NOTFOUND)

## MPI
find_package(MPI REQUIRED CXX)
include_directories(${MPI_CXX_INCLUDE_PATH})

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
