#ifndef __SLIB_UTIL_MATLAB_FUNCTION_H__
#define __SLIB_UTIL_MATLAB_FUNCTION_H__

/*
  This class encapsulates a MATLAB function written as a MATLAB script
  that has been compiled via the MATLAB C/C++ Compiler. The class
  takes care of all of the low-level coordination of objects between a
  MatlabMatrix and an mxArray, which is the object type used by the
  compiled functions/scripts.

  To use a MATLAB function, you need to call the executable
  matlab-build (in this directory) and pass in your MATLAB script as
  the only argument. At the moment, the only functions that are
  supported are functions with a signature like:

  B = function_name(A)

  All input/output variables must be matrices, but they can be any
  type of matrix that MatlabMatrix supports (struct, cell, string,
  numeric).

  Although this signature seems limiting, it is relatively flexible
  because matrices can be comrpised of other matrices, you can
  implement a multi-parameter and multi-output method via:

  B = function(A)

  A1 = A{1};
  A2 = A{2};
  ...
  B1 = ...
  B2 = ...
  ...
  B{1} = B1;
  B{2} = B2;
  ...
 */

#include <mat.h>
#include "matlab.h"
#include <mclmcrrt.h>
#include <string>

#define CREATE_MATLAB_FUNCTION(variable, name)			\
  MatlabFunction variable  = MatlabFunction::Create();		\
  variable.SetHandle(name);					\
  variable.SetInitializer(name ## Initialize);			\
  variable.SetTerminator(name ## Terminate);

typedef bool (*MatlabFunctionHandle)(int nargout, mxArray** B, mxArray* A);
typedef bool (*MatlabInitializer)(void);
typedef void (*MatlabTerminator)(void);

namespace slib {
  namespace util {
    class MatlabFunction {
    public:    
      ~MatlabFunction() {
	if (_terminator) {
	  (*_terminator)();
	}
      }

      static MatlabFunction Create() {
	if (!_initialized) {
	  if (!mclInitializeApplication(NULL, 0)) {
	    _undefined._exists = false;
	    return _undefined;
	  }
	}

	MatlabFunction function;
	return function;
      }

      virtual bool Exists() const {
	return _exists;
      }

      bool Run(MatlabMatrix& A, MatlabMatrix* B) const {
	if (!_handle) {
	  return false;
	}
	if (!_initializer || !(*_initializer)()) {
	  return false;
	}
	
	mxArray* data;
	(*_handle)(1, &data, A._matrix);
	B->AssignData(data);
	return true;
      }

      void SetHandle(MatlabFunctionHandle handle) {
	_handle = handle;
      }

      void SetInitializer(MatlabInitializer handle) {
	_initializer = handle;
      }

      void SetTerminator(MatlabTerminator handle) {
	_terminator = handle;
      }

    protected:
      MatlabFunction() : _handle(NULL), _initializer(NULL), _terminator(NULL), _exists(true) {}
      MatlabFunctionHandle _handle;
      MatlabInitializer _initializer;
      MatlabTerminator _terminator;
      bool _exists;

      static bool _initialized;
      static MatlabFunction _undefined;
    };

    bool MatlabFunction::_initialized = false;
    MatlabFunction MatlabFunction::_undefined;

  }  // namespace util
}  // namespace slib

#define __MATLAB_FUNCTION_CREATOR(name)					\
  class MatlabFunction ## name : public MatlabFunction {		\
  public:								\
  virtual bool RunFunction(const mxArray& A, mxArray* B) const {	\
    if (! ## name ## Initialize()) {					\
      return false;							\
    }									\
    name ## (A, B);							\
    return true;							\
  }									\
  };									


#endif  // __SLIB_UTIL_MATLAB_FUNCTION_H__
