#include "eigs.h"

#include <common/scoped_ptr.h>
#include <glog/logging.h>
#include <mkl_cblas.h>
#include <stdlib.h>
#include <string.h>
#include <util/random.h>

#if  defined(RIOS) && !defined(CLAPACK)
#define F77NAME(x) x
#else
#define F77NAME(x) x ## _
#endif

#define MAX_ARPACK_ITERATIONS 300

typedef int ARint;
typedef int ARlogical;

extern "C" {
  void F77NAME(snaupd)(ARint *ido, char *bmat, ARint *n, char *which,
		       ARint *nev, float *tol, float *resid,
		       ARint *ncv, float *V, ARint *ldv,
		       ARint *iparam, ARint *ipntr, float *workd,
		       float *workl, ARint *lworkl, ARint *info);
  
  void F77NAME(sneupd)(ARlogical *rvec, char *HowMny, ARlogical *select,
		       float *dr, float *di, float *Z,
		       ARint *ldz, float *sigmar,
		       float *sigmai, float *workev, char *bmat,
		       ARint *n, char *which, ARint *nev,
		       float *tol, float *resid, ARint *ncv,
		       float *V, ARint *ldv, ARint *iparam,
		       ARint *ipntr, float *workd, float *workl,
		       ARint *lworkl, ARint *info);
}

using slib::util::Random;

namespace slib {
  namespace matrix {
    
    int EigenSolver::eigs(const FloatMatrix& A, const int& neigs, 
			  FloatMatrix* eigvals, FloatMatrix* eigvecs) {
      if (A.cols() != A.rows()) {
	LOG(ERROR) << "Can only handle square matrices";
	return -1;
      }
      return eigs(A.data(), A.cols(), neigs, eigvals->data(), eigvecs->data());
    }
    
    int EigenSolver::eigs(const float* A, const int& N,
			  const int& neigs, float* eigvals, float* eigvecs) {
      Random::InitializeIfNecessary();
      //float* Af = (float*) MALLOC(sizeof(float) * N * N);
      //ctof(A, N, N, Af);
      
      ARint ido = 0;
      char bmat[1] = {'I'};
      ARint n = N;
      char which[2] = {'L','M'};
      ARint nev = neigs;
      float tol = -1.0f;
      scoped_array<float> resid(new float[n]);
      for (int i = 0; i < n; i++) {
	resid[i] = Random::Uniform(0.0f, 1.0f);
      }
      ARint ncv = 2*nev;
      scoped_array<float> V(new float[n * ncv]);
      memset(V.get(), 0, sizeof(float) * n * ncv);
      ARint ldv = n;
      scoped_array<ARint> iparam(new int[11]);
      memset(iparam.get(), 0, sizeof(ARint) * 11);
      {
	iparam[0] = 1;
	iparam[2] = MAX_ARPACK_ITERATIONS;
	iparam[6] = 1;
      }
      scoped_array<ARint> ipntr(new int[14]);
      memset(ipntr.get(), 0, sizeof(ARint) * 14);
      scoped_array<float> workd(new float[3 * n]);
      memset(workd.get(), 0, sizeof(float) * 3 * n);
      ARint lworkl = 3*ncv*(ncv+2);  // See documentation
      scoped_array<float> workl(new float[lworkl]);
      ARint info_dnaupd = 1;
      
      int iter = 0;
      while(ido != 99) {
	iter = iter + 1;
	//info_dnaupd = 0;
	
	// Repeatedly call the routine DNAUPD and take actions indicated by parameter IDO until
	// either convergence is indicated or maxitr has been exceeded.        
	F77NAME(snaupd)(&ido, bmat, &n, which, &nev, &tol, resid.get(), 
			&ncv, V.get(), &ldv, iparam.get(), ipntr.get(), workd.get(), workl.get(), 
			&lworkl, &info_dnaupd);
	
	if (info_dnaupd != 0) {    
	  // If the IPARAM(5) (fortran addr) is gte to the number of eigs
	  // requested we're still ok
	  if (info_dnaupd == 1) {
	    LOG(WARNING) << "Max iterations reached... " << iparam[4] << " eigenpairs were found";
	  } else {
	    LOG(WARNING) << "Error with snaupd, info = " << info_dnaupd;
	    LOG(WARNING) << ">> Check the documentation of snaupd";
	  }
	  if (info_dnaupd == -9999) {
	    LOG(WARNING) << "Size of current Arnoldi factorization: " << iparam[4];
	  }
	}
	
	if (ido == -1 || ido == 1) {
	  // Perform matrix vector multiplication 
	  cblas_sgemv(CblasRowMajor, CblasNoTrans, N, N, 
		      1.0f, A, N, workd.get()+ipntr[0]-1, 1, 
		      0.0f, workd.get()+ipntr[1]-1, 1);
	  // L O O P   B A C K to call DNAUPD again.
	  continue;
	} else if (ido == 99) {
	} else {
	  LOG(ERROR) << "Received unexpected callback from ARPACK: " << ido;
	  return (ido == 0 ? -1 : ido);
	}
      }
      
      if (info_dnaupd < 0) {
	LOG(ERROR) << "DNAUPD returned an error: " << info_dnaupd;
	return info_dnaupd;
      } 
      // Post-Process using DNEUPD.
      ARlogical rvec = 1;
      char howmany[1] = {'A'};
      scoped_array<ARlogical> select(new ARlogical[ncv]);
      scoped_array<float> dr(new float[nev + 1]);
      scoped_array<float> di(new float[nev + 1]);
      float sigmar;
      float sigmai;
      scoped_array<float> workev(new float[3 * ncv]);
      ARint info_dneupd;
      
      info_dneupd = 0;
      
      F77NAME(sneupd)(&rvec, howmany, select.get(), dr.get(), di.get(), V.get(), &ldv, 
		      &sigmar, &sigmai, workev.get(), 
		      bmat, &n, which, &nev, &tol, resid.get(), 
		      &ncv, V.get(), &ldv, iparam.get(), ipntr.get(), workd.get(), workl.get(), 
		      &lworkl, &info_dneupd);
      
      if (info_dneupd != 0) {
	LOG(WARNING) << "Error with sneupd, info = " << info_dneupd;
	LOG(WARNING) << ">> Check the documentation of sneupd.";
      }
      
      // Print additional convergence information.
      if (info_dneupd == 1) {
	LOG(INFO) << "Maximum number of iterations reached.";
      } else if (info_dneupd == 3) {
	LOG(INFO) << "No shifts could be applied during implicit Arnoldi update, try increasing NCV.";
      }
      
      for (int i = 0; i < neigs; i++) {
	eigvals[i] = dr[i];
      }
      
      for (int i = 0; i < neigs; i++) {
	for (int j = 0; j < N; j++) {
	  eigvecs[j + i*N] = V[j + i*N];
	}
      }
      
#ifdef DEBUG
      printf("\nEIGS\n");
      printf("======\n");
      printf("\n");
      printf("Iterations is %d\n", iter);
      printf("Size of the matrix is %d\n", n);
      printf("The number of Ritz values requested is %d\n", nev);
      printf("The number of Arnoldi vectors generated (NCV) is %d\n", ncv);
      printf("What portion of the spectrum: %c%c\n", which[0], which[1]);
      printf("The number of Implicit Arnoldi update iterations taken is %d\n", iparam[2]);
      printf("The number of OP*x is %d\n", iparam[8]);
      printf("The convergence criterion is %f\n", tol);
#endif  
            
      return 0;
    }
  }  // namespace matrix
}  // namespace slib
