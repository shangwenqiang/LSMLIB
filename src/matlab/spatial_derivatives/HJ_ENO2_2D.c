/*
 * File:        HJ_ENO2_2D.c
 * Copyright:   (c) 2005-2008 Kevin T. Chu and Masa Prodanovic
 * Revision:    $Revision$
 * Modified:    $Date$
 * Description: MATLAB MEX-file for 2d, second-order plus and minus HJ ENO 
 */

/*=======================================================================
 *
 * HJ_ENO2_2D() computes the second-order plus and minus HJ ENO
 * approximation to grad(phi).
 *
 * Usage: [phi_x_plus, phi_y_plus, phi_x_minus, phi_y_minus] = ...
 *        HJ_ENO2_2D(phi, ghostcell_width, dX)
 *
 * Arguments:
 * - phi:              function for which to compute plus and minus
 *                       spatial derivatives
 * - ghostcell_width:  number of ghostcells at boundary of
 *                       computational domain
 * - dX:               array containing the grid spacing
 *                       in coordinate directions
 *
 * Return values:
 * - phi_x_plus:       x-component of second-order, plus
 *                       HJ ENO derivative
 * - phi_y_plus:       y-component of second-order, plus
 *                       HJ ENO derivative
 * - phi_x_minus:      x-component of second-order, minus
 *                       HJ ENO derivative
 * - phi_y_minus:      y-component of second-order, minus
 *                       HJ ENO derivative
 *
 * NOTES:
 * - phi_x_plus, phi_y_plus, phi_x_minus and phi_y_minus have the
 *   same ghostcell width as phi.
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
#define GHOSTCELL_WIDTH (prhs[1])
#define DX              (prhs[2])

/* Output Arguments */ 
#define PHI_X_PLUS      (plhs[0])
#define PHI_Y_PLUS      (plhs[1])
#define PHI_X_MINUS     (plhs[2])
#define PHI_Y_MINUS     (plhs[3])


void mexFunction( int nlhs, mxArray *plhs[], 
		  int nrhs, const mxArray*prhs[] )
     
{ 
  LSMLIB_REAL *phi_x_plus, *phi_x_minus, *phi_y_plus, *phi_y_minus;
  int ilo_grad_phi_gb, ihi_grad_phi_gb, jlo_grad_phi_gb, jhi_grad_phi_gb;
  LSMLIB_REAL *phi; 
  int ilo_phi_gb, ihi_phi_gb, jlo_phi_gb, jhi_phi_gb;
  LSMLIB_REAL *D1;
  int ilo_D1_gb, ihi_D1_gb, jlo_D1_gb, jhi_D1_gb;
  LSMLIB_REAL *D2;
  int ilo_D2_gb, ihi_D2_gb, jlo_D2_gb, jhi_D2_gb;
  int ilo_fb, ihi_fb, jlo_fb, jhi_fb;
  double *dX;
  LSMLIB_REAL dX_meshgrid_order[2];
  int ghostcell_width;
  int num_data_array_dims;
  
  /* Check for proper number of arguments */
  if (nrhs != 3) { 
    mexErrMsgTxt("Three required input arguments."); 
  } else if (nlhs > 4) {
    mexErrMsgTxt("Too many output arguments."); 
  } 
    
  /* Parameter Checks */
  num_data_array_dims = mxGetNumberOfDimensions(PHI);
  if (num_data_array_dims != 2) {
    mexErrMsgTxt("phi should be a 2 dimensional array."); 
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

  /* Get ghostcell_width */
  ghostcell_width = mxGetPr(GHOSTCELL_WIDTH)[0];

  /* Get dX */
  dX = mxGetPr(DX);

  /* Change order of dX to match MATLAB meshgrid() order for grids. */
  dX_meshgrid_order[0] = dX[1];
  dX_meshgrid_order[1] = dX[0];

  /* Assign pointers for phi */
  phi = (LSMLIB_REAL*) mxGetPr(PHI);
      
  /* Get size of phi data */
  ilo_phi_gb = 1;
  ihi_phi_gb = mxGetM(PHI);
  jlo_phi_gb = 1;
  jhi_phi_gb = mxGetN(PHI);

  /* Create matrices for plus and minus derivatives */
  ilo_grad_phi_gb = ilo_phi_gb;
  ihi_grad_phi_gb = ihi_phi_gb;
  jlo_grad_phi_gb = ilo_phi_gb;
  jhi_grad_phi_gb = jhi_phi_gb;
#ifdef LSMLIB_DOUBLE_PRECISION
  PHI_X_PLUS = mxCreateDoubleMatrix(ihi_grad_phi_gb-ilo_grad_phi_gb+1,
                                    jhi_grad_phi_gb-jlo_grad_phi_gb+1,
                                    mxREAL);
  PHI_Y_PLUS = mxCreateDoubleMatrix(ihi_grad_phi_gb-ilo_grad_phi_gb+1,
                                    jhi_grad_phi_gb-jlo_grad_phi_gb+1,
                                    mxREAL);
  PHI_X_MINUS = mxCreateDoubleMatrix(ihi_grad_phi_gb-ilo_grad_phi_gb+1,
                                    jhi_grad_phi_gb-jlo_grad_phi_gb+1,
                                    mxREAL);
  PHI_Y_MINUS = mxCreateDoubleMatrix(ihi_grad_phi_gb-ilo_grad_phi_gb+1,
                                    jhi_grad_phi_gb-jlo_grad_phi_gb+1,
                                    mxREAL);
#else
  PHI_X_PLUS = mxCreateNumericMatrix(ihi_grad_phi_gb-ilo_grad_phi_gb+1,
                                     jhi_grad_phi_gb-jlo_grad_phi_gb+1,
                                     mxSINGLE_CLASS, mxREAL);
  PHI_Y_PLUS = mxCreateNumericMatrix(ihi_grad_phi_gb-ilo_grad_phi_gb+1,
                                     jhi_grad_phi_gb-jlo_grad_phi_gb+1,
                                     mxSINGLE_CLASS, mxREAL);
  PHI_X_MINUS = mxCreateNumericMatrix(ihi_grad_phi_gb-ilo_grad_phi_gb+1,
                                     jhi_grad_phi_gb-jlo_grad_phi_gb+1,
                                     mxSINGLE_CLASS, mxREAL);
  PHI_Y_MINUS = mxCreateNumericMatrix(ihi_grad_phi_gb-ilo_grad_phi_gb+1,
                                     jhi_grad_phi_gb-jlo_grad_phi_gb+1,
                                     mxSINGLE_CLASS, mxREAL);
#endif
  phi_x_plus  = (LSMLIB_REAL*) mxGetPr(PHI_X_PLUS); 
  phi_y_plus  = (LSMLIB_REAL*) mxGetPr(PHI_Y_PLUS); 
  phi_x_minus = (LSMLIB_REAL*) mxGetPr(PHI_X_MINUS); 
  phi_y_minus = (LSMLIB_REAL*) mxGetPr(PHI_Y_MINUS); 


  /* Allocate scratch memory for undivided differences */
  ilo_D1_gb = ilo_phi_gb;
  ihi_D1_gb = ihi_phi_gb;
  jlo_D1_gb = ilo_phi_gb;
  jhi_D1_gb = jhi_phi_gb;
  D1 = (LSMLIB_REAL*) mxMalloc( sizeof(LSMLIB_REAL)
                              * (ihi_D1_gb-ilo_D1_gb+1) 
                              * (jhi_D1_gb-jlo_D1_gb+1) );

  ilo_D2_gb = ilo_phi_gb;
  ihi_D2_gb = ihi_phi_gb;
  jlo_D2_gb = ilo_phi_gb;
  jhi_D2_gb = jhi_phi_gb;
  D2 = (LSMLIB_REAL*) mxMalloc( sizeof(LSMLIB_REAL)
                              * (ihi_D2_gb-ilo_D2_gb+1) 
                              * (jhi_D2_gb-jlo_D2_gb+1) );

  if ( (!D1) || (!D2) ) {
    if (D1) mxFree(D1);
    if (D2) mxFree(D2);
    mexErrMsgTxt("Unable to allocate memory for scratch data...aborting....");
  }

  /* Do the actual computations in a Fortran 77 subroutine */
  ilo_fb = ilo_phi_gb+ghostcell_width;
  ihi_fb = ihi_phi_gb-ghostcell_width;
  jlo_fb = jlo_phi_gb+ghostcell_width;
  jhi_fb = jhi_phi_gb-ghostcell_width;

  /* 
   * NOTE: ordering of data arrays from meshgrid() is (y,x), so order
   * derivative data arrays needs to be reversed.
   */
  LSM2D_HJ_ENO2(
    phi_y_plus, phi_x_plus, 
    &ilo_grad_phi_gb, &ihi_grad_phi_gb,
    &jlo_grad_phi_gb, &jhi_grad_phi_gb,
    phi_y_minus, phi_x_minus, 
    &ilo_grad_phi_gb, &ihi_grad_phi_gb,
    &jlo_grad_phi_gb, &jhi_grad_phi_gb,
    phi, 
    &ilo_phi_gb, &ihi_phi_gb, &jlo_phi_gb, &jhi_phi_gb, 
    D1,
    &ilo_D1_gb, &ihi_D1_gb, &jlo_D1_gb, &jhi_D1_gb, 
    D2,
    &ilo_D2_gb, &ihi_D2_gb, &jlo_D2_gb, &jhi_D2_gb, 
    &ilo_fb, &ihi_fb, &jlo_fb, &jhi_fb,
    &dX_meshgrid_order[0], &dX_meshgrid_order[1]);

  /* Deallocate scratch memory for undivided differences */
  mxFree(D1);
  mxFree(D2);

  return;
}
