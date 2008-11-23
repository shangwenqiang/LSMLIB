/*
 * File:        computeDistanceFunction3d.c
 * Copyright:   (c) 2005-2008 Kevin T. Chu and Masa Prodanovic
 * Revision:    $Revision: 1.11 $
 * Modified:    $Date: 2006/09/18 16:17:01 $
 * Description: MATLAB MEX-file for using the fast marching method to
 *              compute the distance function for 3d level set functions
 */

/*===========================================================================
 *
 * computeDistanceFunction3d() computes a distance from an 
 * arbitrary level set function in three-dimensions using the 
 * Fast Marching Method.
 * 
 * Usage: distance_fcn = ...
 *          computeDistanceFunction3d(phi, dX, ...
 *                                    mask, ...
 *                                    spatial_discretization_order)
 *
 * Arguments:
 * - phi:                           level set function to use in 
 *                                  computing distance function
 * - dX:                            array containing the grid spacing
 *                                  in each coordinate direction
 * - mask:                          mask for domain of problem;
 *                                  grid points outside of the domain
 *                                  of the problem should be set to a
 *                                  negative value
 *                                  (default = [])
 * - spatial_discretization_order:  order of discretization for 
 *                                  spatial derivatives
 *                                  (default = 2)
 *
 * Return value:
 * - distance_fcn:                  distance function
 *
 * NOTES:
 * - All data arrays are assumed to be in the order generated by the
 *   MATLAB meshgrid() function.  That is, data corresponding to the
 *   point (x_i,y_j,z_k) is stored at index (j,i,k).
 *
 *===========================================================================*/

#include "mex.h"
#include "LSMLIB_config.h"
#include "lsm_fast_marching_method.h" 

/* Input Arguments */
#define PHI	                      (prhs[0])
#define DX                        (prhs[1])
#define MASK                      (prhs[2])
#define SPATIAL_DERIVATIVE_ORDER  (prhs[3])

/* Output Arguments */
#define DISTANCE_FUNCTION         (plhs[0])

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  /* field data */
  LSMLIB_REAL *phi;
  LSMLIB_REAL *mask;
  LSMLIB_REAL *distance_fcn;
 
  /* grid data */
  const int *grid_dims = mxGetDimensions(PHI);
  double *dX = mxGetPr(DX);
  LSMLIB_REAL dX_matlab_order[3];

  /* numerical parameters */
  int spatial_discretization_order;

  /* auxilliary variables */
  int i;
  int error_code;
  mxArray* tmp_mxArray;

  /* Check for proper number of arguments */
  if (nrhs < 2) {
    mexErrMsgTxt(
      "Insufficient number of input arguments (2 required; 2 optional)");
  } else if (nrhs > 4) {
    mexErrMsgTxt("Too many input arguments (2 required; 2 optional)");
  } else if (nlhs > 1) {
    mexErrMsgTxt("Too many output arguments.");
  }

  /* Check that the inputs have the correct floating-point precision */
#ifdef LSMLIB_DOUBLE_PRECISION
    if (!mxIsDouble(PHI)) {
      mexErrMsgTxt("Incompatible precision: LSMLIB built for double-precision but phi is single-precision");
    }
#else
    if (!mxIsSingle(PHI)) {
      mexErrMsgTxt("Incompatible precision: LSMLIB built for single-precision but phi is double-precision");
    }
#endif

  /* Get mask */
  if ( (nrhs < 3) || (mxIsEmpty(MASK)) ) {
    mask = 0;  /* NULL mask ==> all points are in interior of domain */
  } else {

    /* Check that mask has proper precision */
#ifdef LSMLIB_DOUBLE_PRECISION
    if (!mxIsDouble(MASK)) {
      mexErrMsgTxt("Incompatible precision: LSMLIB built for double-precision but mask is single-precision");
    }
#else
    if (!mxIsSingle(MASK)) {
      mexErrMsgTxt("Incompatible precision: LSMLIB built for single-precision but mask is double-precision");
    }
#endif

    mask = (LSMLIB_REAL*) mxGetPr(MASK);
  }
  
  /* Get spatial derivative order */
  if (nrhs < 4) {
    spatial_discretization_order = 2; 
  } else {
    spatial_discretization_order = mxGetPr(SPATIAL_DERIVATIVE_ORDER)[0];
  }

  /* Assign pointers for phi data */
  phi = (LSMLIB_REAL*) mxGetPr(PHI);

  /* Create distance function data */
#ifdef LSMLIB_DOUBLE_PRECISION
  DISTANCE_FUNCTION = mxCreateNumericArray(3, grid_dims, 
                                           mxDOUBLE_CLASS, mxREAL);
#else
  DISTANCE_FUNCTION = mxCreateNumericArray(3, grid_dims, 
                                           mxSINGLE_CLASS, mxREAL);
#endif
  distance_fcn = (LSMLIB_REAL*) mxGetPr(DISTANCE_FUNCTION);

  /* Change order of dX to be match MATLAB meshgrid() order for grids. */
  dX_matlab_order[0] = dX[1];
  dX_matlab_order[1] = dX[0];
  dX_matlab_order[2] = dX[2];

  /* Carry out FMM calculation */
  error_code = computeDistanceFunction3d(
                 distance_fcn,
                 phi,
                 mask,
                 spatial_discretization_order,
                 (int*) grid_dims,
                 dX_matlab_order);

  if (error_code) {
    mexErrMsgTxt("computeDistanceFunction3d failed...");
  }

  return;
}
