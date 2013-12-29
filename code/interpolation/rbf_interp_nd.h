#ifndef __SLIB_INTERPOLATION_RBF_INTERP_ND_H__
#define __SLIB_INTERPOLATION_RBF_INTERP_ND_H__

/*
Richard Franke,
Scattered Data Interpolation: Tests of Some Methods,
Mathematics of Computation,
Volume 38, Number 157, January 1982, pages 181-200.

William Press, Brian Flannery, Saul Teukolsky, William Vetterling,
Numerical Recipes in FORTRAN: The Art of Scientific Computing,
Third Edition,
Cambridge University Press, 2007,
ISBN13: 978-0-521-88068-8,
LC: QA297.N866.
*/

double *rbf_interp_nd ( int m, int nd, double xd[], double r0, 
  void phi ( int n, double r[], double r0, double v[] ), double w[], 
  int ni, double xi[] );

double *rbf_weight ( int m, int nd, double xd[], double r0, 
  void phi ( int n, double r[], double r0, double v[] ), 
  double fd[] );

#endif  // __SLIB_INTERPOLATION_RBF_INTERP_ND_H__
