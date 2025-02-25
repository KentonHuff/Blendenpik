#include "mex.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "config.h"

#ifdef BLAS_UNDERSCORE
#define DGEMV dgemv_
#define DTRSV dtrsv_
#define DNRM2 dnrm2_
#else
#define DGEMV dgemv
#define DTRSV dtrsv
#define DNRM2 dnrm2
#endif
#include "blas.h"


//double DNRM2(long *, double *, long *);


void scale(double *x, ptrdiff_t n, double s)
{
  int i;

  for(i = 0; i < n; i++)
    x[i] *= s;
}

void mexFunction(int nargout, mxArray *argout[], int nargin, const mxArray *argin[]) 
{
  double tol;
  double *A, *b, *R, *x, *u, *v, *vt, *d, *z;
  double beta, alpha, normr, normar, norma;
  double c, s, phibar, phi, nn;
  double thet, rhot, rho;

  double *lsvec, *resvec, *Atr, *r;

  ptrdiff_t m, n;
  ptrdiff_t maxit, it, i;

  ptrdiff_t int_zero = 0;
  ptrdiff_t int_one = 1;
  double dbl_one = 1.0;
  double dbl_mone = -1.0;
  double dbl_zero = 0.0;


  A = mxGetPr(argin[0]);
  b = mxGetPr(argin[1]);
  R = mxIsEmpty(argin[2]) ? NULL : mxGetPr(argin[2]);

  tol = mxGetScalar(argin[3]);
  maxit = (int)mxGetScalar(argin[4]);

  m = mxGetM(argin[0]);
  n = mxGetN(argin[0]);

  u = malloc(m * sizeof(double));
  v = malloc(n * sizeof(double));
  vt = malloc(n * sizeof(double));
  d = malloc(n * sizeof(double));
  z = malloc(n * sizeof(double));

  argout[0] = mxCreateDoubleMatrix(n, 1, mxREAL);
  x = mxGetPr(argout[0]);

  if (nargout > 2) {
    argout[2] = mxCreateDoubleMatrix(maxit+1, 1, mxREAL);
    lsvec = mxGetPr(argout[2]);
    argout[3] = mxCreateDoubleMatrix(maxit+1, 1, mxREAL);
    resvec = mxGetPr(argout[3]);

    Atr = malloc(n * sizeof(double));
    r = malloc(m * sizeof(double));
    memcpy(r, b, m * sizeof(double));

    DGEMV("T", &m, &n, &dbl_one, A, &m, r, &int_one, &dbl_zero, Atr, &int_one);
    resvec[0] = DNRM2(&m, r, &int_one);
    lsvec[0] = DNRM2(&n, Atr, &int_one);
  }

  memset(x, 0, n * sizeof(double));
  memset(d, 0, n * sizeof(double));
  memcpy(u, b, m * sizeof(double));
  beta = DNRM2(&m, u, &int_one);
  normr = beta;
  scale(u, m, 1/beta);
  c = 1; s = 0; phibar = beta;
  DGEMV("T", &m, &n, &dbl_one, A, &m, u, &int_one, &dbl_zero, v, &int_one);
  if (R != NULL)
    DTRSV("U", "T", "Not Unit", &n, R, &n, v, &int_one);

  alpha = DNRM2(&n, v, &int_one);
  scale(v, n, 1/alpha);
	
  normar = alpha * beta;
  norma = 0;

  it = 0;
  while (it < maxit) {
    double malpha = -alpha;
    memcpy(z, v, n * sizeof(double));
    if (R != NULL)
      DTRSV("U", "N", "Not Unit", &n, R, &n, z, &int_one);
    DGEMV("N", &m, &n, &dbl_one, A, &m, z, &int_one, &malpha, u, &int_one);
    beta = DNRM2(&m, u, &int_one);
    scale(u, m, 1/beta);

    norma = sqrt(norma * norma + alpha * alpha + beta * beta);

    thet = - s * alpha;
    rhot = c * alpha;
    rho = sqrt(rhot * rhot + beta * beta);
    c = rhot / rho;
    s = - beta / rho;
    phi = c * phibar;
    phibar = s * phibar;
		
    for (i = 0; i < n; i++) {      
      d[i] = (z[i] - thet * d[i]) / rho;
      x[i] = x[i] + phi * d[i];
    }
    it++;

    if (nargout > 2) {
      memcpy(r, b, m * sizeof(double));
      DGEMV("N", &m, &n,  &dbl_mone, A, &m, x, &int_one, &dbl_one, r, &int_one);
      DGEMV("T", &m, &n, &dbl_one, A, &m, r, &int_one, &dbl_zero, Atr, &int_one);
      lsvec[it] = DNRM2(&n, Atr, &int_one);
      resvec[it] = DNRM2(&m, r, &int_one);
    }

    normr = fabs(s) * normr;
    nn = normar / (normr * norma);

    if (nn < tol)
      break;

    DGEMV("T", &m, &n, &dbl_one, A, &m, u, &int_one, &dbl_zero, vt, &int_one);
    if (R != NULL)
      DTRSV("U", "T", "Not Unit", &n, R, &n, vt, &int_one);
    for (i = 0; i < n; i++)
      v[i] = vt[i] - beta * v[i];

    alpha = DNRM2(&n, v, &int_one);
    scale(v, n, 1/alpha);	

    normar = alpha * fabs(s * phi);
  }

  if (nargout > 2){
    mxSetM(argout[2] , it + 1);
    mxSetM(argout[3] , it + 1);
  }

  if (nargout > 1) 
    argout[1] = mxCreateDoubleScalar(it);
  if (nn > tol)
    mexPrintf("dense_lsqr: did not converge\n");
  else
    mexPrintf("dense_lsqr: converged at iteration %d\n", it);

  free(u);
  free(v);
  free(vt);
  free(d);
  free(z);
  if (nargout > 2) {
    free(Atr);
    free(r);
  }
}
