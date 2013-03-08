#ifdef __MACH__
#include <cblas.h>
#else
#include <mkl_cblas.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "eigs.h"
#include "util.h"
#include "rand.h"

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

// Fortran matrices are in column-major order
void ctof(const float* cMatrix, const int& rows, const int& cols, float* fMatrix) {
  for (int j = 0; j < rows; j++) {
    for (int i = 0; i < cols; i++) {
      fMatrix[IDX2(j,i,rows)] = cMatrix[IDX2(i,j,cols)];
    }
  }
}

int EigenSolver::eigs(const float* A, const int& N,
		      const int& neigs, float* eigvals, float* eigvecs) {
  initRandom();
  //float* Af = (float*) MALLOC(sizeof(float) * N * N);
  //ctof(A, N, N, Af);
  
  ARint ido = 0;
  char bmat[1] = {'I'};
  ARint n = N;
  char which[2] = {'L','M'};
  ARint nev = neigs;
  float tol = -1.0f;
  float* resid = (float*) MALLOC(sizeof(float) * n);
  for (int i = 0; i < n; i++) {
    resid[i] = genUniformRandom(0.0f, 1.0f);
  }
  ARint ncv = 2*nev;
  float* V = (float*) MALLOC(sizeof(float) * n * ncv);
  memset(V, 0, sizeof(float) * n * ncv);
  ARint ldv = n;
  ARint* iparam = (int*) MALLOC(sizeof(int) * 11);
  memset(iparam, 0, sizeof(ARint) * 11);
  {
    iparam[0] = 1;
    iparam[2] = MAX_ARPACK_ITERATIONS;
    iparam[6] = 1;
  }
  ARint* ipntr = (int*) MALLOC(sizeof(int) * 14);
  memset(ipntr, 0, sizeof(ARint) * 14);
  float* workd = (float*) MALLOC(sizeof(float) * 3 * n);
  memset(workd, 0, sizeof(float) * 3 * n);
  ARint lworkl = 3*ncv*(ncv+2);  // See documentation
  float* workl = (float*) MALLOC(sizeof(float) * lworkl);
  ARint info_dnaupd = 1;
  
  int iter = 0;
  while(ido != 99) {
    iter = iter + 1;
    //info_dnaupd = 0;
    
    // Repeatedly call the routine DNAUPD and take actions indicated by parameter IDO until
    // either convergence is indicated or maxitr has been exceeded.        
    F77NAME(snaupd)(&ido, bmat, &n, which, &nev, &tol, resid, 
		    &ncv, V, &ldv, iparam, ipntr, workd, workl, 
		    &lworkl, &info_dnaupd);

    if (info_dnaupd != 0) {    
      // If the IPARAM(5) (fortran addr) is gte to the number of eigs
      // requested we're still ok
      if (info_dnaupd == 1) {
	printf("\nMax iterations reached... %d eigenpairs were found\n", iparam[4]);
      } else {
	printf("\nError with snaupd, info = %d\n", info_dnaupd);
	printf("Check the documentation of snaupd.\n\n");
      }
      if (info_dnaupd == -9999) {
	PRINTLN("Size of current Arnoldi factorization: %d", iparam[4]);
      }
    }
    
    if (ido == -1 || ido == 1) {
      // Perform matrix vector multiplication 
      cblas_sgemv(CblasRowMajor, CblasNoTrans, N, N, 
		  1.0f, A, N, workd+ipntr[0]-1, 1, 
		  0.0f, workd+ipntr[1]-1, 1);
      // L O O P   B A C K to call DNAUPD again.
      continue;
    } else if (ido == 99) {
    } else {
      ERROR_ALL("Received unexpected callback from ARPACK: %d", ido);
      return (ido == 0 ? -1 : ido);
    }
  }
  
  if (info_dnaupd < 0) {
    ERROR_ALL("DNAUPD returned an error: %d", info_dnaupd);
    return info_dnaupd;
  } 
  // Post-Process using DNEUPD.
  ARlogical rvec = 1;
  char howmany[1] = {'A'};
  ARlogical* select = (ARlogical*) MALLOC(sizeof(ARlogical) * ncv);
  float* dr = (float*) MALLOC(sizeof(float) * nev + 1);
  float* di = (float*) MALLOC(sizeof(float) * nev + 1);
  float sigmar;
  float sigmai;
  float* workev = (float*) MALLOC(sizeof(float) * 3 * ncv);
  ARint info_dneupd;
  
  info_dneupd = 0;
  
  F77NAME(sneupd)(&rvec, howmany, select, dr, di, V, &ldv, &sigmar, &sigmai, workev, 
		  bmat, &n, which, &nev, &tol, resid, 
		  &ncv, V, &ldv, iparam, ipntr, workd, workl, 
		  &lworkl, &info_dneupd);

  if (info_dneupd != 0) {
    printf("\nError with sneupd, info = %d\n", info_dneupd);
    printf("Check the documentation of sneupd.\n\n");
  }
  
  // Print additional convergence information.
  if (info_dneupd == 1) {
    printf("\nMaximum number of iterations reached.\n\n");
  } else if (info_dneupd == 3) {
    printf("\nNo shifts could be applied during implicit Arnoldi update, try increasing NCV.\n\n");
  }
  
  for (int i = 0; i < neigs; i++) {
    eigvals[i] = dr[i];
  }
  
  for (int i = 0; i < neigs; i++) {
    for (int j = 0; j < N; j++) {
      eigvecs[IDX2(j,i,N)] = V[IDX2(j,i,N)];
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

  FREE(resid);
  FREE(V);
  FREE(iparam);
  FREE(ipntr);
  FREE(workd);
  FREE(workl);

  FREE(select);
  FREE(dr);
  FREE(di);
  FREE(workev);
  
  return 0;
}
