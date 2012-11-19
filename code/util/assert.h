#ifndef __SLIB_ASSERT_H__
#define __SLIB_ASSERT_H__

#include <fstream>
#include <iostream>

using std::cerr;
using std::cout;
using std::endl;

#define ASSERT_NOT_NULL(a) {						\
    if (a == NULL) {							\
      cerr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__	\
	   << ":: Assertion Failed! Expected " << #a << " not NULL"	\
	   << endl;							\
      exit(1);								\
    }									\
  }

#define ASSERT_EQ(a, b) {						\
    if (a != b) {							\
      cerr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__	\
	   << ":: Assertion Failed! Expected " << #a << " (" << a	\
	   << ") = " << #b << " (" << b << ")" << endl;			\
      exit(1);								\
    }									\
  }

#define ASSERT_LT(a, b) {						\
    if (a >= b) {							\
      cerr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__	\
	   << ":: Assertion Failed! Expected " << #a << " (" << a	\
	   << ") < " << #b << " (" << b << ")" << endl;			\
      exit(1);								\
    }									\
  }

#define ASSERT_GT(a, b) {						\
    if (a <= b) {							\
      cerr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__	\
	   << ":: Assertion Failed! Expected " << #a << " (" << a	\
	   << ") > " << #b << " (" << b << ")" << endl;			\
      exit(1);								\
    }									\
  }

#define ASSERT_LTE(a, b) {						\
    if (a > b) {							\
      cerr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__	\
	   << ":: Assertion Failed! Expected " << #a << " (" << a	\
	   << ") <= " << #b << " (" << b << ")" << endl;		\
      exit(1);								\
    }									\
  }

#define ASSERT_GTE(a, b) {						\
    if (a < b) {							\
      cerr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__	\
	   << ":: Assertion Failed! Expected " << #a << " (" << a	\
	   << ") >= " << #b << " (" << b << ")" << endl;		\
      exit(1);								\
    }									\
  }

#endif
