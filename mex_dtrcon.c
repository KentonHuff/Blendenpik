#include "mex.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/timeb.h>

#include "config.h"

#ifdef BLAS_UNDERSCORE
#define DTRCON dtrcon_
#else
#define DTRCON dtrcon
#endif
#include "lapack.h"

double wtime()
{
  struct timeb T;

  static time_t time_diff;
  static time_t mill_diff;

  double dt;

  (void) ftime( &T );

  time_diff = T.time;
  mill_diff = T.millitm;

  dt = ((double) time_diff) + (1e-3) * ((double) mill_diff);

  return dt;
}

void mexFunction(int nargout, mxArray *argout[], int nargin, const mxArray *argin[])
{
  ptrdiff_t n, ld;
  double rcond;
  double *A, *workspace1;
  ptrdiff_t *workspace2;
  ptrdiff_t info = 0;

  ld = mxGetM(argin[0]);
  n = mxGetN(argin[0]);
  A = mxGetPr(argin[0]);

  /* Compute */
  workspace1 = (double *)mxMalloc(3 * n * sizeof(double));
  workspace2 = (ptrdiff_t *)mxMalloc(n * sizeof(ptrdiff_t));
  DTRCON("1", "U", "N", &n, A, &ld, &rcond, workspace1, workspace2, &info);
  argout[0] = mxCreateDoubleScalar(rcond);
  mxFree(workspace1);
  mxFree(workspace2);
}
