#ifndef __SLIB_ASSERT_H__
#define __SLIB_ASSERT_H__

#include "cesium/cesium.h"
#include <glog/logging.h>

#define ASSERT_NOT_NULL(a) {						\
    if (a == NULL) {							\
      LOG(ERROR) << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__	\
		 << ":: Assertion Failed! Expected "			\
		 << #a << " not NULL";					\
      if (slib::cesium::Cesium::HasStarted()) {				\
	slib::cesium::Cesium::GetInstance()->Abort(1);			\
      } else {								\
	exit(1);							\
      }									\
    }									\
  }

#define ASSERT_TRUE(a) {						\
    if (a == NULL) {							\
      LOG(ERROR) << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__	\
		 << ":: Assertion Failed! Expected "			\
		 << #a << " TRUE";					\
      if (slib::cesium::Cesium::HasStarted()) {				\
	slib::cesium::Cesium::GetInstance()->Abort(1);			\
      } else {								\
	exit(1);							\
      }									\
    }									\
  }


#define ASSERT_EQ(a, b) {						\
    if (a != b) {							\
      LOG(ERROR) << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__	\
		 << ":: Assertion Failed! Expected " << #a << " (" << a	\
		 << ") = " << #b << " (" << b << ")";			\
      if (slib::cesium::Cesium::HasStarted()) {				\
	slib::cesium::Cesium::GetInstance()->Abort(1);			\
      } else {								\
	exit(1);							\
      }									\
    }									\
  }

#define ASSERT_LT(a, b) {						\
    if (a >= b) {							\
      LOG(ERROR) << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__	\
		 << ":: Assertion Failed! Expected " << #a << " (" << a	\
		 << ") < " << #b << " (" << b << ")";			\
      if (slib::cesium::Cesium::HasStarted()) {				\
	slib::cesium::Cesium::GetInstance()->Abort(1);			\
      } else {								\
	exit(1);							\
      }									\
    }									\
  }

#define ASSERT_GT(a, b) {						\
    if (a <= b) {							\
      LOG(ERROR) << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__	\
		 << ":: Assertion Failed! Expected " << #a << " (" << a	\
		 << ") > " << #b << " (" << b << ")";			\
      if (slib::cesium::Cesium::HasStarted()) {				\
	slib::cesium::Cesium::GetInstance()->Abort(1);			\
      } else {								\
	exit(1);							\
      }									\
    }									\
  }

#define ASSERT_LTE(a, b) {						\
    if (a > b) {							\
      LOG(ERROR) << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__	\
		 << ":: Assertion Failed! Expected " << #a << " (" << a	\
		 << ") <= " << #b << " (" << b << ")";			\
      if (slib::cesium::Cesium::HasStarted()) {				\
	slib::cesium::Cesium::GetInstance()->Abort(1);			\
      } else {								\
	exit(1);							\
      }									\
    }									\
  }

#define ASSERT_GTE(a, b) {						\
    if (a < b) {							\
      LOG(ERROR) << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__	\
		 << ":: Assertion Failed! Expected " << #a << " (" << a	\
		 << ") >= " << #b << " (" << b << ")";			\
      if (slib::cesium::Cesium::HasStarted()) {				\
	slib::cesium::Cesium::GetInstance()->Abort(1);			\
      } else {								\
	exit(1);							\
      }									\
    }									\
  }

#endif
