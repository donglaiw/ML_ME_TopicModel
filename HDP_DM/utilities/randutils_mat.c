#ifndef RANDUTILS
#define RANDUTILS
#include "mex.h"
#include <math.h>
#include <stdlib.h>

double matlab_rand(int row,int col)
{
/* Calling Matlab's random number*/
    double *rand;
    mxArray *lhs[1],*rhs[2];
    rhs[0]=mxCreateDoubleScalar(row);
    rhs[1]=mxCreateDoubleScalar(col);
    mexCallMATLAB(1, lhs, 2,rhs , "rand");
    rand=mxGetPr(lhs[0]);
    return rand[0];
}

double randgamma(double rr) {
  double aa, bb, cc, dd;
  double uu, vv, ww, xx, yy, zz;

  if ( rr <= 0.0 ) {
    /* Not well defined, set to zero and skip. */
    return 0.0;
  } else if ( rr == 1.0 ) {
    /* Exponential */
    return - log(matlab_rand(1,1));
   /* return - log(drand48());*/
  } else if ( rr < 1.0 ) {
    /* Use Johnks generator */
    cc = 1.0 / rr;
    dd = 1.0 / (1.0-rr);
    while (1) {
      /*
      xx = pow(drand48(), cc);
      yy = xx + pow(drand48(), dd);
      */      
      xx = pow(matlab_rand(1,1), cc);
      yy = xx + pow(matlab_rand(1,1), dd);

      if ( yy <= 1.0 ) {
        /*return -log(drand48()) * xx / yy;*/
        return -log(matlab_rand(1,1)) * xx / yy;
      }
    }
  } else { /* rr > 1.0 */
    /* Use bests algorithm */
    bb = rr - 1.0;
    cc = 3.0 * rr - 0.75;
    while (1) {
      /*
      uu = drand48();
      vv = drand48();
      */
      uu = matlab_rand(1,1);
      vv = matlab_rand(1,1);
      ww = uu * (1.0 - uu);
      yy = sqrt(cc / ww) * (uu - 0.5);
      xx = bb + yy;
      if (xx >= 0) {
        zz = 64.0 * ww * ww * ww * vv * vv;
        if ( ( zz <= (1.0 - 2.0 * yy * yy / xx) ) ||
             ( log(zz) <= 2.0 * (bb * log(xx / bb) - yy) ) ) {
          return xx;
        }
      }
    }
  }
}

int randnumtable(double alpha, int numdata) {
  int ii, numtable;

  if ( numdata == 0 ) {
    return 0;
  } else {
    numtable = 1;
    for ( ii = 1 ; ii < numdata ; ii++ ) {
      /*if ( drand48() < alpha / (ii+alpha) ) numtable++;*/
      if ( matlab_rand(1,1) < alpha / (ii+alpha) ) numtable++;
    }
    return numtable;
  }
} 

void randdir(double *pi, double *alpha, int veclength, int skip) {
  double *pi2, *piend;
  double sum;

  sum = 0.0;
  piend = pi + veclength*skip;
  for ( pi2 = pi ; pi2 < piend ; pi2 += skip) {
    sum += *pi2 = randgamma(*alpha);
    alpha += skip;
  }
  for ( pi2 = pi ; pi2 < piend ; pi2 += skip) {
    *pi2 /= sum;
  }
}

double randbeta(double aa, double bb) {
  aa = randgamma(aa);
  bb = randgamma(bb);
  return aa/(aa+bb);
}

int randmult(double *pi, int veclength, int skip) {
  double *pi2, *piend;
  double sum = 0.0, mass;
  int cc = 0;

  piend = pi + veclength*skip;
  for ( pi2 = pi ; pi2 < piend ; pi2 += skip )
    {sum += *pi2;/*printf("ss:%f\n",sum);*/}
  /*mass = drand48() * sum;*/
    mass =  matlab_rand(1,1)* sum;
    /*printf("rr %f,,",mass);*/
  while (1) {
    /*printf("bb %f,%f,%d\n",mass,*pi,cc);*/
    mass -= *pi;
    if ( mass <= 0.0 ) break;
    pi += skip;
    cc ++;
  }
  return cc;
}

int randuniform(int numvalue) {
   return floor(matlab_rand(1,1) * numvalue);
  /*return floor(drand48() * numvalue);*/
}

double randconparam(double alpha, int numgroup, int *numdata, int *numtable,
                    double alphaa, double alphab, int numiter) {
  int iter, jj, nd, zz;
  double aa, bb, xx;
  for ( iter = 0 ; iter < numiter ; iter++ ) {
    aa = alphaa;
    bb = alphab;
    for ( jj = 0 ; jj < numgroup ; jj++ ) {
      nd = numdata[jj];
      xx = randbeta(alpha+1.0, nd);
      /*zz = ( drand48() * (alpha + nd) < nd );*/
      zz = ( matlab_rand(1,1) * (alpha + nd) < nd );
      
      aa += numtable[jj] - zz;
      bb -= log(xx);
    }
    alpha = randgamma(aa) / bb;
  }
  return alpha;
}

#endif
