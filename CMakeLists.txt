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
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w1 -wd2196 -wd2536 -wd780 -mkl -Qoption,cpp,--rvalue_ctor_is_not_copy_ctor")
endif(INTEL_CC MATCHES INTEL_CC-NOTFOUND)

option(DEBUG_SYMBOLS "DEBUG_SYMBOLS" OFF)

if(DEBUG_SYMBOLS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -std=c++0x")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++0x")
endif()

add_definitions(-DSKIP_OPENCV)

include_directories(./thirdparty/CImg)
include_directories(./code)

set(CMAKE_MODULE_PATH ./cmake)

set(MODULES util string image interpolation svm cesium matrix kmeans)

# Required libraries

## Boost
# SLIB_REQUIRED Boost pkg libboost-all-dev
find_package(Boost COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

## X11 (not required)
find_package(X11)
if (X11_FOUND)
  include_directories(${X11_INCLUDE_DIR})
  list (APPEND TEST_LIBRARIES ${X11_LIBRARIES})
else ()
  set_property (GLOBAL PROPERTY COMPILE_DEFINITIONS cimg_display=0)
endif ()

## Eigen3
# SLIB_REQUIRED Eigen3 bundle thirdparty/eigen
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
# SLIB_REQUIRED MATLAB bundle thirdparty/matlab
if(DEFINED ENV{MATLAB_INCLUDE_DIR} AND EXISTS "$ENV{MATLAB_INCLUDE_DIR}/mat.h")
  include_directories($ENV{MATLAB_INCLUDE_DIR})
else()
  add_definitions(-DDISABLE_MATLAB)
  message(WARNING "\nMATLAB installation not found. It will be disabled.\n\nIf you have MATLAB installed, please set the environment variable MATLAB_INCLUDE_DIR to the include directory of your MATLAB installation. This is typically something like /usr/local/MATLAB/R2011b/extern/include/.")
endif()

## Google Flags
# SLIB_REQUIRED GoogleFlags bundle thirdparty/gflags
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
# SLIB_REQUIRED GoogleLogging bundle thirdparty/glog
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
find_package(FFTW3 QUIET)
if (FFTW3_FOUND)
  include_directories(${FFTW3_INCLUDE_DIRS})
else ()
  message(WARNING "\nCould not find FFTW3 library. FFTW support will be disabled.")
  add_definitions(-DSKIP_FFTW)
endif()

## BLAS
find_package(BLAS)
if(BLAS_FOUND)
  include_directories(${BLAS_INCLUDE_DIR})

  if (NOT ${BLAS_INFO} MATCHES "mkl")
    add_definitions(-DDISABLE_MKL_BLAS)

    if(NOT EXISTS "${BLAS_INCLUDE_DIR}/cblas.h")
      message(WARNING "\nCould not find CBLAS library. Fast eigenvector decomposition will be disabled.")
      add_definitions(-DDISABLE_EIGS)
    endif()
  endif()
else()
    message(WARNING "\nCould not find CBLAS library. Fast eigenvector decomposition will be disabled.")
    add_definitions(-DDISABLE_EIGS)
endif()

## MPI
# SLIB_REQUIRED MPI bundle thirdparty/mpich
# SLIB_CHECK MPI "mpicc -v 2>&1 | grep -o 1.4.1p1"
find_package(MPI REQUIRED CXX)
include_directories(${MPI_CXX_INCLUDE_PATH})
add_definitions(${MPI_CXX_COMPILE_FLAGS})
add_definitions(-DMPICH_SKIP_MPICXX)
add_definitions(-DOMPI_SKIP_MPICXX)

# CUDA (This is mainly for Caffe so we disable Caffe if we can't find it)
find_package(CUDA QUIET)
if (CUDA_FOUND)
  include_directories(${CUDA_INCLUDE_DIRS})
else ()
  message(WARNING "\nCould not find CUDA library. Caffe features will be disabled.")
  add_definitions(-DSKIP_CAFFE_FEATURE_COMPUTER)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ./lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ./lib)

option(BUILD_SHARED "BUILD_SHARED" OFF)

# Compile each module
set(TEST_FILES)
foreach(MODULE ${MODULES}) 

  file(GLOB ALL_FILES 
    "./code/${MODULE}/*.cc"
    "./code/${MODULE}/*.cpp"
    )

  set(FILES)
  foreach(FILE ${ALL_FILES})
    if(${FILE} MATCHES "test")
      list(APPEND TEST_FILES ${FILE})
    else(${FILE} MATCHES "test")
      list(APPEND FILES ${FILE})
    endif(${FILE} MATCHES "test")
  endforeach(FILE)

  if(BUILD_SHARED)
    add_library(${MODULE} SHARED ${FILES})
  else()
    add_library(${MODULE} STATIC ${FILES})
  endif()
  
  set_target_properties(${MODULE} PROPERTIES LINKER_LANGUAGE CXX)
  
endforeach(MODULE) 

option(SKIP_TESTS "SKIP_TESTS" OFF)

if(NOT SKIP_TESTS)

######################################################################################
######################################################################################

# Build all the tests
if(DEFINED ENV{MATLAB_LIB_DIR})
  include_directories($ENV{MATLAB_INCLUDE_DIR})

  string (REGEX REPLACE "bin" "sys/os" MATLAB_OS_DIR $ENV{MATLAB_LIB_DIR})
  
  set (MATLAB_MAT_LIB "$ENV{MATLAB_LIB_DIR}/libmat.so")
  
  add_library(matlab_mat SHARED IMPORTED)
  set_property(TARGET matlab_mat PROPERTY
    IMPORTED_LOCATION ${MATLAB_MAT_LIB})
  list (APPEND TEST_LIBRARIES matlab_mat)  
  
  set (MATLAB_MX_LIB "$ENV{MATLAB_LIB_DIR}/libmx.so")
  
  add_library(matlab_mx SHARED IMPORTED)
  set_property(TARGET matlab_mx PROPERTY
    IMPORTED_LOCATION ${MATLAB_MX_LIB})
  list (APPEND TEST_LIBRARIES matlab_mx)  

else()
  message(FATAL_ERROR "\nTo build tests, please set the environment variable MATLAB_INCLUDE_DIR and MATLAB_LIB_DIR to the include/lib directory of your MATLAB installation. This is typically something like /usr/local/MATLAB/R2011b/extern/{include,lib/glxna64}. You can skip this if you don't want to build any tests, but it is worth setting anyway when it comes time to build your own binaries.")
endif()

find_package(MPI REQUIRED CXX)
include_directories(${MPI_CXX_INCLUDE_PATH})
list (APPEND TEST_LIBRARIES ${MPI_CXX_LIBRARIES})
add_definitions(${MPI_CXX_COMPILE_FLAGS})

add_definitions(-DMPICH_SKIP_MPICXX)
add_definitions(-DOMPI_SKIP_MPICXX)

## Google Flags
if(DEFINED ENV{GFLAGS_INCLUDE_DIR} AND DEFINED ENV{GFLAGS_LIB_DIR})
  include_directories($ENV{GFLAGS_INCLUDE_DIR})
  link_directories ($ENV{GFLAGS_LIB_DIR})
else()
  find_package(GoogleFlags)
  if(GFLAGS_FOUND)
    include_directories(${GFLAGS_INCLUDE_DIR})
  else(GFLAGS_FOUND)
    message(FATAL_ERROR "\nCouldn't automatically locate your Google Flags installation. Please set the environment variables GFLAGS_INCLUDE_DIR and GFLAGS_LIB_DIR to the correct directories.")
  endif(GFLAGS_FOUND)
endif()

## Google Logging
if(DEFINED ENV{GLOG_INCLUDE_DIR} AND DEFINED ENV{GLOG_LIB_DIR})
  include_directories($ENV{GLOG_INCLUDE_DIR})
  link_directories ($ENV{GLOG_LIB_DIR})
else()
  find_package(GoogleLogging)
  if(GLOG_FOUND)
    include_directories(${GLOG_INCLUDE_DIR})
  else(GLOG_FOUND)
    message(FATAL_ERROR "\nCouldn't automatically locate your Google Logging installation. Please set the environment variables GLOG_INCLUDE_DIR and GLOG_LIB_DIR to the correct directories.")
  endif(GLOG_FOUND)
endif()

## FFTW3
find_package(FFTW3)
if (FFTW3_FOUND)
  include_directories(${FFTW3_INCLUDE_DIRS})
  link_directories (${FFTW3_LIB_DIRS})

  list (APPEND TEST_LIBRARIES fftw3f)
endif()

## Boost
if(DEFINED ENV{BOOST_INCLUDE_DIR})
  include_directories($ENV{BOOST_INCLUDE_DIR})
else(DEFINED ENV{BOOST_INCLUDE_DIR})
  find_package(Boost)
  if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    include_directories(${BOOST_INCLUDE_DIRS})
  else()
    message(FATAL_ERROR "\nCouldn't automatically locate your Boost installation directory. Please set the environment variable BOOST_INCLUDE_DIR to the correct directory.")
  endif()
endif(DEFINED ENV{BOOST_INCLUDE_DIR})

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
if(DEFINED ENV{MATLAB_INCLUDE_DIR} AND DEFINED ENV{MATLAB_LIB_DIR})
  include_directories($ENV{MATLAB_INCLUDE_DIR})
  link_directories ($ENV{MATLAB_LIB_DIR})

  string (REGEX REPLACE "bin" "sys/os" MATLAB_OS_DIR $ENV{MATLAB_LIB_DIR})

  if (DEFINED ENV{TACC_SYSTEM})
    # Need the stdc++ library from MATLAB
    file (GLOB files "${MATLAB_OS_DIR}/*libstdc++*.so.*")
    list (GET files 0 MATLAB_CSTD_LIB)
    
    add_library(matlab_stdc++ SHARED IMPORTED)
    set_property(TARGET matlab_stdc++ PROPERTY
      IMPORTED_LOCATION ${MATLAB_CSTD_LIB})
    list (APPEND TEST_LIBRARIES matlab_stdc++)  
  endif()

else()
  message(FATAL_ERROR "\nPlease set the environment variable MATLAB_INCLUDE_DIR and MATLAB_LIB_DIR to the include/lib directory of your MATLAB installation. This is typically something like /usr/local/MATLAB/R2011b/extern/{include,lib/glxna64}")
endif()

list (APPEND TEST_LIBRARIES gfortran glog gflags jpeg stdc++)

add_library(slib_svm STATIC IMPORTED)
set_property(TARGET slib_svm PROPERTY
  IMPORTED_LOCATION lib/libsvm.a)
add_library(slib_cesium STATIC IMPORTED)
set_property(TARGET slib_cesium PROPERTY
  IMPORTED_LOCATION lib/libcesium.a)
add_library(slib_util STATIC IMPORTED)
set_property(TARGET slib_util PROPERTY
  IMPORTED_LOCATION lib/libutil.a)
add_library(slib_string STATIC IMPORTED)
set_property(TARGET slib_string PROPERTY
  IMPORTED_LOCATION lib/libstring.a)
add_library(slib_image STATIC IMPORTED)
set_property(TARGET slib_image PROPERTY
  IMPORTED_LOCATION lib/libimage.a)
add_library(slib_kmeans STATIC IMPORTED)
set_property(TARGET slib_kmeans PROPERTY
  IMPORTED_LOCATION lib/libkmeans.a)
add_library(slib_matrix STATIC IMPORTED)
set_property(TARGET slib_matrix PROPERTY
  IMPORTED_LOCATION lib/libmatrix.a)
add_library(slib_interpolation STATIC IMPORTED)
set_property(TARGET slib_interpolation PROPERTY
  IMPORTED_LOCATION lib/libinterpolation.a)

list (APPEND TEST_LIBRARIES -Wl,-start-group slib_svm slib_cesium slib_util slib_string slib_image slib_kmeans slib_interpolation slib_matrix -Wl,-end-group)

## ARPACK
find_package(ARPACK)
if(ARPACK_FOUND)
  list (APPEND TEST_LIBRARIES ${ARPACK_LIB})
else()
  message(WARNING "\nCouldn't find an installation of ARPACK on this machine. Compilation is still possible, but fast eigen decomposition routines for large/sparse matrices will not be possible.")
  add_definitions(-DOSKIP_ARPACK)
endif()

file(MAKE_DIRECTORY "tests")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "tests")
set(TEST_BINARIES)
foreach(TEST_FILE ${TEST_FILES})

  GET_FILENAME_COMPONENT(BINARY ${TEST_FILE} NAME_WE)

  # Explicitly avoiding some troublesome binaries that need to be
  # built with special care
  if(${BINARY} STREQUAL "test_matlab_function")
  elseif(${BINARY} STREQUAL "test_eigs" AND (NOT ARPACK_FOUND))
  else()
    add_executable(${BINARY} EXCLUDE_FROM_ALL ${TEST_FILE} ${LINKED_SOURCES})
    target_link_libraries (${BINARY} ${TEST_LIBRARIES})
    list(APPEND TEST_BINARIES ${BINARY})
  endif()
endforeach()

add_custom_target(tests DEPENDS ${TEST_BINARIES})

endif()
