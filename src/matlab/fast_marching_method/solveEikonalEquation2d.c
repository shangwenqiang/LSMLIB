/*
 * File:        solveEikonalEquation2d.c
 * Copyright:   (c) 2005-2008 Kevin T. Chu and Masa Prodanovic
 * Revision:    $Revision$
 * Modified:    $Date$
 * Description: MATLAB MEX-file for using the fast marching method to
 *              solve the Eikonal equation in 2d
 */

/*===========================================================================
 *
 * solveEikonalEquation2d() solves the Eikonal equation
 *
 *   |grad(phi)| = 1/speed(x,y)
 *
 * in two-dimensions using the Fast Marching Method.
 *
 * Usage: phi = solveEikonalEquation2d(boundary_data, speed, dX, ...
 *                                     mask, spatial_discretization_order)
 *
 * Arguments:
 * - boundary_data:                 data array containing boundary data and
 *                                  domain information.  The value at grid
 *                                  points adjacent to the boundary of the
 *                                  domain should be set to the desired
 *                                  positive values; the value at all other
 *                                  grid points should be negative.
 * - speed:                         speed function in Eikonal equation
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
 * - phi:                           solution to Eikonal equation
 *
 * NOTES:
 * - It is the user's responsibility to set the boundary_data for phi. 
 *   Grid points where the value of phi are to be solved for MUST be 
 *   set to a negative value. 
 *
 * - All data arrays are assumed to be in the order generated by the
 *   MATLAB meshgrid() function.  That is, data corresponding to the
 *   point (x_i,y_j) is stored at index (j,i).
 *
 * - It is NOT necessary to mask out regions where the speed is zero.
 *   Grid points where the speed is zero are automatically treated as
 *   being outside of the domain.
 * 
 * - boundary_data, speed, and mask MUST have the same grid dimensions.
 *
 *===========================================================================*/

#include "mex.h"
#include "LSMLIB_config.h"
#include "lsm_fast_marching_method.h" 

/* Input Arguments */
#define BOUNDARY_DATA             (prhs[0])
#define SPEED                     (prhs[1])
#define DX                        (prhs[2])
#define MASK                      (prhs[3])
#define SPATIAL_DERIVATIVE_ORDER  (prhs[4])

/* Output Arguments */
#define PHI                       (plhs[0])

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  /* field data */
  LSMLIB_REAL *boundary_data;
  LSMLIB_REAL *phi;
  LSMLIB_REAL *speed;
  LSMLIB_REAL *mask;
 
  /* grid data */
  const int *grid_dims_phi = mxGetDimensions(BOUNDARY_DATA);
  const int *grid_dims_speed = mxGetDimensions(SPEED);
  int num_gridpts;
  double *dX = mxGetPr(DX);
  LSMLIB_REAL dX_matlab_order[2];

  /* numerical parameters */
  int spatial_discretization_order;

  /* auxilliary variables */
  int i;
  int error_code;
  mxArray* tmp_mxArray;

  /* Check for proper number of arguments */
  if (nrhs < 3) {
    mexErrMsgTxt(
      "Insufficient number of input arguments (3 required; 2 optional)");
  } else if (nrhs > 5) {
    mexErrMsgTxt("Too many input arguments (3 required; 2 optional)");
  } else if (nlhs > 1) {
    mexErrMsgTxt("Too many output arguments.");
  }

  /* Check for that grid dimensions are the same */
  if ( (grid_dims_phi[0] != grid_dims_speed[0]) ||
       (grid_dims_phi[1] != grid_dims_speed[1]) ) {
    mexErrMsgTxt("The dimensions for phi and speed should be the same.");
  }

  /* Check that the inputs have the correct floating-point precision */
#ifdef LSMLIB_DOUBLE_PRECISION
    if (!mxIsDouble(BOUNDARY_DATA)) {
      mexErrMsgTxt("Incompatible precision: LSMLIB built for double-precision but boundary_data is single-precision");
    }
    if (!mxIsDouble(SPEED)) {
      mexErrMsgTxt("Incompatible precision: LSMLIB built for double-precision but speed is single-precision");
    }
#else
    if (!mxIsSingle(BOUNDARY_DATA)) {
      mexErrMsgTxt("Incompatible precision: LSMLIB built for single-precision but boundary_data is double-precision");
    }
    if (!mxIsSingle(SPEED)) {
      mexErrMsgTxt("Incompatible precision: LSMLIB built for single-precision but speed is double-precision");
    }
#endif

  /* Get mask */
  if ( (nrhs < 4) || (mxIsEmpty(MASK)) ) {
    mask = 0;  /* NULL mask ==> all points are in interior of domain */
  } else {

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
  if (nrhs < 5) {
    spatial_discretization_order = 2;  
  } else {
    spatial_discretization_order = mxGetPr(SPATIAL_DERIVATIVE_ORDER)[0];
  }
  
  /* Assign pointers for boundary_data and speed data */
  boundary_data = (LSMLIB_REAL*) mxGetPr(BOUNDARY_DATA);
  speed = (LSMLIB_REAL*) mxGetPr(SPEED);

  /* Create and initialize PHI data */
#ifdef LSMLIB_DOUBLE_PRECISION
  PHI  = mxCreateDoubleMatrix(grid_dims_phi[0], grid_dims_phi[1], mxREAL);
#else
  PHI = mxCreateNumericMatrix(grid_dims_phi[0], grid_dims_phi[1],
                              mxSINGLE_CLASS, mxREAL);
#endif
  phi = (LSMLIB_REAL*) mxGetPr(PHI);
  num_gridpts = grid_dims_phi[0]*grid_dims_phi[1];
  for (i = 0; i < num_gridpts; i++) {
    phi[i] = boundary_data[i];
  }

  /* Change order of dX to be consistent with MATLAB order for grids. */
  dX_matlab_order[0] = dX[1];
  dX_matlab_order[1] = dX[0];

  /* Carry out FMM calculation */
  error_code = solveEikonalEquation2d(
                 phi,
                 speed,
                 mask,
                 spatial_discretization_order,
                 (int*) grid_dims_phi,
                 dX_matlab_order);

  if (error_code) {
    mexErrMsgTxt("solveEikonalEquation2d failed...");
  }

  return;
}
