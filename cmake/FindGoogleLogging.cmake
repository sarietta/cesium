# - Try to find Google Logging lib
#
#  GLOG_FOUND - system has Google Logging lib
#  GLOG_INCLUDE_DIR - the Google Logging include directory
#  GLOG_LIBRARY - the Google Logging library

find_path(GLOG_INCLUDE_DIR NAMES logging.h
  HINTS
  ${CMAKE_INSTALL_PREFIX}/include
  ${CMAKE_SYSTEM_INCLUDE_PATH}/include
  ${KDE4_INCLUDE_DIR}
  PATH_SUFFIXES glog)

find_library(GLOG_LIBRARY NAMES glog
  HINTS
  ${CMAKE_INSTALL_PREFIX}/include
  ${CMAKE_SYSTEM_INCLUDE_PATH}/include
  ${KDE4_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GoogleLogging DEFAULT_MSG GLOG_LIBRARY GLOG_INCLUDE_DIR)

set (GLOG_FOUND TRUE)
if (GLOG_LIBRARY MATCHES GLOG_LIBRARY-NOTFOUND)
  set (GLOG_FOUND FALSE)
endif (GLOG_LIBRARY MATCHES GLOG_LIBRARY-NOTFOUND)
if (GLOG_INCLUDE_DIR MATCHES GLOG_INCLUDE_DIR-NOTFOUND)
  set (GLOG_FOUND FALSE)
endif (GLOG_INCLUDE_DIR MATCHES GLOG_INCLUDE_DIR-NOTFOUND)

mark_as_advanced(GLOG_INCLUDE_DIR GLOG_LIBRARY)
