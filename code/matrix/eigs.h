#ifndef __EIGS_H__
#define __EIGS_H__

#if  defined(RIOS) && !defined(CLAPACK)
#define F77NAME(x) x
#else
#define F77NAME(x) x ## _
#endif

// Type conversion.

typedef int ARint;
typedef int ARlogical;

class EigenSolver {
public:
  static int eigs(const float* A, const int& N,
		  const int& neigs, float* eigvals, float* eigvecs);
};

#endif
