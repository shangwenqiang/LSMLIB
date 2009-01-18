/*
 * File:        UPWIND_HJ_ENO1_2D.c
 * Copyright:   (c) 2005-2008 Kevin T. Chu and Masa Prodanovic
 * Revision:    $Revision$
 * Modified:    $Date$
 * Description: MATLAB MEX-file for 2d, first-order upwind HJ ENO 
 */

/*=======================================================================
 *
 * UPWIND_HJ_ENO1_2D() computes the first-order upwind HJ ENO
 * approximation to grad(phi)
 *
 * Usage: [phi_x, phi_y] = ...
 *        UPWIND_HJ_ENO1_2D(phi, vel_x, vel_y, ghostcell_width, dX)
 *
 * Arguments:
 * - phi:              function for which to compute upwind
 *                       derivative
 * - vel_x:            x-component of velocity to use in upwinding
 * - vel_y:            y-component of velocity to use in upwinding
 * - ghostcell_width:  number of ghostcells at boundary of
 *                       computational domain
 * - dX:               array containing the grid spacing
 *                       in coordinate directions
 *
 * Return values:
 * - phi_x:            x-component of first-order, upwind 
 *                       HJ ENO derivative
 * - phi_y:            y-component of first-order, upwind 
 *                       HJ ENO derivative
 *
 * NOTES:
 * - The vel_x and vel_y arrays are assumed to be the same size.
 *
 * - phi_x and phi_y have the same ghostcell width as phi.
 *
 * - All data arrays are assumed to be in the order generated by the
 *   MATLAB meshgrid() function.  That is, data corresponding to the
 *   point (x_i,y_j) is stored at index (j,i).  The output data arrays
 *   will be returned with the same ordering as the input data arrays.
 *
 *=======================================================================*/

#include "mex.h"
#include "matrix.h"
#include "LSMLIB_config.h"
#include "lsm_spatial_derivatives2d.h"

/* Input Arguments */ 
#define PHI             (prhs[0])
#define VEL_X           (prhs[1])
#define VEL_Y           (prhs[2])
#define GHOSTCELL_WIDTH (prhs[3])
#define DX              (prhs[4])

/* Output Arguments */ 
#define PHI_X           (plhs[0])
#define PHI_Y           (plhs[1])


void mexFunction( int nlhs, mxArray *plhs[], 
		  int nrhs, const mxArray*prhs[] )
     
{ 
  LSMLIB_REAL *phi_x, *phi_y;
  int ilo_grad_phi_gb, ihi_grad_phi_gb, jlo_grad_phi_gb, jhi_grad_phi_gb;
  LSMLIB_REAL *phi; 
  int ilo_phi_gb, ihi_phi_gb, jlo_phi_gb, jhi_phi_gb;
  LSMLIB_REAL *vel_x, *vel_y;
  int ilo_vel_gb, ihi_vel_gb, jlo_vel_gb, jhi_vel_gb;
  LSMLIB_REAL *D1;
  int ilo_D1_gb, ihi_D1_gb, jlo_D1_gb, jhi_D1_gb;
  int ilo_fb, ihi_fb, jlo_fb, jhi_fb;
  double *dX;
  LSMLIB_REAL dX_meshgrid_order[2];
  int ghostcell_width;
  int num_data_array_dims;
  
  /* Check for proper number of arguments */
  if (nrhs != 5) { 
    mexErrMsgTxt("Five required input arguments."); 
  } else if (nlhs > 2) {
    mexErrMsgTxt("Too many output arguments."); 
  } 
    
  /* Parameter Checks */
  num_data_array_dims = mxGetNumberOfDimensions(PHI);
  if (num_data_array_dims != 2) {
    mexErrMsgTxt("phi should be a 2 dimensional array."); 
  }
  num_data_array_dims = mxGetNumberOfDimensions(VEL_X);
  if (num_data_array_dims != 2) {
    mexErrMsgTxt("vel_x should be a 2 dimensional array."); 
  }
  num_data_array_dims = mxGetNumberOfDimensions(VEL_Y);
  if (num_data_array_dims != 2) {
    mexErrMsgTxt("vel_y should be a 2 dimensional array."); 
  }

  /* Check that the inputs have the correct floating-point precision */
#ifdef LSMLIB_DOUBLE_PRECISION
    if (!mxIsDouble(PHI)) {
      mexErrMsgTxt("Incompatible precision: LSMLIB built for double-precision but phi is single-precision");
    }
    if (!mxIsDouble(VEL_X)) {
      mexErrMsgTxt("Incompatible precision: LSMLIB built for double-precision but vel_x is single-precision");
    }
    if (!mxIsDouble(VEL_Y)) {
      mexErrMsgTxt("Incompatible precision: LSMLIB built for double-precision but vel_y is single-precision");
    }
#else
    if (!mxIsSingle(PHI)) {
      mexErrMsgTxt("Incompatible precision: LSMLIB built for single-precision but phi is double-precision");
    }
    if (!mxIsSingle(VEL_X)) {
      mexErrMsgTxt("Incompatible precision: LSMLIB built for single-precision but vel_x is double-precision");
    }
    if (!mxIsSingle(VEL_Y)) {
      mexErrMsgTxt("Incompatible precision: LSMLIB built for single-precision but vel_y is double-precision");
    }
#endif

  /* Get ghostcell_width */
  ghostcell_width = mxGetPr(GHOSTCELL_WIDTH)[0];

  /* Get dX */
  dX = mxGetPr(DX);

  /* Change order of dX to be match MATLAB meshgrid() order for grids. */
  dX_meshgrid_order[0] = dX[1];
  dX_meshgrid_order[1] = dX[0];

  /* Assign pointers for phi and velocities */
  phi = (LSMLIB_REAL*) mxGetPr(PHI);
  vel_x = (LSMLIB_REAL*) mxGetPr(VEL_X);
  vel_y = (LSMLIB_REAL*) mxGetPr(VEL_Y);
      
  /* Get size of phi data */
  ilo_phi_gb = 1;
  ihi_phi_gb = mxGetM(PHI);
  jlo_phi_gb = 1;
  jhi_phi_gb = mxGetN(PHI);

  /* Get size of velocity data (assume vel_x and vel_y have same size) */
  ilo_vel_gb = 1;
  ihi_vel_gb = mxGetM(VEL_X);
  jlo_vel_gb = 1;
  jhi_vel_gb = mxGetN(VEL_X);

  /* if necessary, shift ghostbox for velocity to be */
  /* centered with respect to the ghostbox for phi.  */
  if (ihi_vel_gb != ihi_phi_gb) {
    int shift = (ihi_phi_gb-ihi_vel_gb)/2;
    ilo_vel_gb += shift;
    ihi_vel_gb += shift;
  }
  if (jhi_vel_gb != jhi_phi_gb) {
    int shift = (jhi_phi_gb-jhi_vel_gb)/2;
    jlo_vel_gb += shift;
    jhi_vel_gb += shift;
  }

  /* Create matrices for upwind derivatives (i.e. phi_x and phi_y) */
  ilo_grad_phi_gb = ilo_phi_gb;
  ihi_grad_phi_gb = ihi_phi_gb;
  jlo_grad_phi_gb = ilo_phi_gb;
  jhi_grad_phi_gb = jhi_phi_gb;
#ifdef LSMLIB_DOUBLE_PRECISION
  PHI_X = mxCreateDoubleMatrix(ihi_grad_phi_gb-ilo_grad_phi_gb+1,
                               jhi_grad_phi_gb-jlo_grad_phi_gb+1,
                               mxREAL);
  PHI_Y = mxCreateDoubleMatrix(ihi_grad_phi_gb-ilo_grad_phi_gb+1,
                               jhi_grad_phi_gb-jlo_grad_phi_gb+1,
                               mxREAL);
#else
  PHI_X = mxCreateNumericMatrix(ihi_grad_phi_gb-ilo_grad_phi_gb+1,
                                jhi_grad_phi_gb-jlo_grad_phi_gb+1,
                                mxSINGLE_CLASS, mxREAL);
  PHI_Y = mxCreateNumericMatrix(ihi_grad_phi_gb-ilo_grad_phi_gb+1,
                                jhi_grad_phi_gb-jlo_grad_phi_gb+1,
                                mxSINGLE_CLASS, mxREAL);
#endif
  phi_x = (LSMLIB_REAL*) mxGetPr(PHI_X); 
  phi_y = (LSMLIB_REAL*) mxGetPr(PHI_Y); 


  /* Allocate scratch memory for undivided differences */
  ilo_D1_gb = ilo_phi_gb;
  ihi_D1_gb = ihi_phi_gb;
  jlo_D1_gb = ilo_phi_gb;
  jhi_D1_gb = jhi_phi_gb;
  D1 = (LSMLIB_REAL*) mxMalloc( sizeof(LSMLIB_REAL)
                              * (ihi_D1_gb-ilo_D1_gb+1) 
                              * (jhi_D1_gb-jlo_D1_gb+1) );
  if (!D1) {
    mexErrMsgTxt("Unable to allocate memory for scratch data...aborting....");
  }


  /* Do the actual computations in a Fortran 77 subroutine */
  ilo_fb = ilo_phi_gb+ghostcell_width;
  ihi_fb = ihi_phi_gb-ghostcell_width;
  jlo_fb = jlo_phi_gb+ghostcell_width;
  jhi_fb = jhi_phi_gb-ghostcell_width;

  /* 
   * NOTE: ordering of data arrays from meshgrid() is (y,x), so order
   * derivative and velocity data arrays need to be reversed.
   */
  LSM2D_UPWIND_HJ_ENO1(
    phi_y, phi_x, 
    &ilo_grad_phi_gb, &ihi_grad_phi_gb,
    &jlo_grad_phi_gb, &jhi_grad_phi_gb,
    phi, 
    &ilo_phi_gb, &ihi_phi_gb, &jlo_phi_gb, &jhi_phi_gb, 
    vel_y, vel_x, 
    &ilo_vel_gb, &ihi_vel_gb, &jlo_vel_gb, &jhi_vel_gb,
    D1,
    &ilo_D1_gb, &ihi_D1_gb, &jlo_D1_gb, &jhi_D1_gb, 
    &ilo_fb, &ihi_fb, &jlo_fb, &jhi_fb,
    &dX_meshgrid_order[0], &dX_meshgrid_order[1]);

  /* Deallocate scratch memory for undivided differences */
  mxFree(D1);

  return;
}
