/*
 * File:        PatchModule.h
 * Copyright:   (c) 2005-2008 Kevin T. Chu and Masa Prodanovic
 * Revision:    $Revision$
 * Modified:    $Date$
 * Description: Header for concrete subclass of LevelSetMethodPatchStrategy 
 *              that computes the single patch numerical routines for the 
 *              level set method example problem
 */

#ifndef included_PatchModule
#define included_PatchModule

/************************************************************************
 *
 * This PatchModule class provides routines for initializing
 * the level set function.  It does not implement any boundary conditions
 * because it is assumed that periodic boundary conditions are used for
 * the level set function.
 *
 ************************************************************************/

   
// SAMRAI configuration header must be included
// before any other SAMRAI header files
#include "SAMRAI_config.h"

#include <string>
#include "IntVector.h"
#include "Patch.h"
#include "tbox/Database.h"
#include "tbox/Pointer.h"

// LevelSetMethod configuration header must be included
// before any other LevelSetMethod header files
#include "LSMLIB_config.h"
#include "LevelSetMethodPatchStrategy.h"
#include "LevelSetMethodVelocityFieldStrategy.h"

// SAMRAI namespaces
using namespace SAMRAI;
using namespace hier;
using namespace tbox;
using namespace LSMLIB;

class PatchModule:
  public LevelSetMethodPatchStrategy<2>
{
public:

  /*!
   * Enumeration of Initial Level Set Configurations
   */
  typedef enum { CIRCLE = 0, LOBES = 1 } INITIAL_SET_TYPE;

  /*!
   * This constructor sets the object name and reads in the 
   * user input from the specified input database.
   *
   * Arguments:
   *  - input_db (in):         pointer to database containing user input
   *  - object_name (in):      string name for object
   *
   * Return value:             none
   *
   */
  PatchModule(
    Pointer<Database> input_db,
    const string& object_name = "PatchModule");

  /*!
   * Empty destructor.
   */
  virtual ~PatchModule() {};


  /****************************************************************
   *
   * Methods Inherited from LevelSetMethodPatchStrategy
   *
   ****************************************************************/

  /*!
   * initializeLevelSetFunctionsOnPatch() initializes the level set
   * function on the patch based on the value of 
   * "initial_level_set" in the input database.
   *
   * Arguments:
   *  - input_db (in):         pointer to database containing user input
   *  - patch_hierarchy (in):  PatchHierarchy on which to compute velocity field
   *  - grid_geometry (in):    geometry of the computational grid
   *  - object_name (in):      string name for object
   *
   * Return value:             none
   *
   */
  virtual void initializeLevelSetFunctionsOnPatch(Patch<2>& patch,
                                                  const LSMLIB_REAL data_time,
                                                  const int phi_handle,
                                                  const int psi_handle);

  /*!
   * setLevelSetFunctionBoundaryConditions() sets the data in ghost cells 
   * corresponding to physical boundary conditions.  
   */
  virtual void setLevelSetFunctionBoundaryConditions(
    Patch<2>& patch,
    const LSMLIB_REAL fill_time,
    const int phi_handle,
    const int psi_handle,
    const IntVector<2>& ghost_width_to_fill);

  /*!
   * Print all data members for FluidSolver class.
   */
  void printClassData(ostream& os) const;

protected:

  /****************************************************************
   *
   * Utility Methods
   *
   ****************************************************************/

  void getFromInput(Pointer<Database> db);


  /*
   * The object name is used for error/warning reporting and also as a
   * string label for restart database entries.
   */
  string d_object_name;

  /*
   * d_initial_level_set is set by the initial_level_set field in the
   * input database.
   */
  int d_initial_level_set;

  /*
   * other initial level set parameters
   */
  LSMLIB_REAL d_radius;
  LSMLIB_REAL d_center[2];
  int d_num_lobes;

  /*
   * class constants
   */
  static const LSMLIB_REAL s_default_radius;
  static const int s_default_num_lobes;
};

#endif
